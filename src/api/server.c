#include "api/server.h"

#include "common/strings.h"
#include "core/runtime.h"
#include "logging/logger.h"

#if CHARNESS_HAVE_LIBUV && CHARNESS_HAVE_CJSON

#if __has_include(<uv.h>)
#include <uv.h>
#endif

#if __has_include(<cjson/cJSON.h>)
#include <cjson/cJSON.h>
#elif __has_include(<cJSON.h>)
#include <cJSON.h>
#endif

#include <stdlib.h>
#include <string.h>

typedef struct charness_api_client_context {
    uv_tcp_t handle;
    uv_write_t write_request;
    char *response;
} charness_api_client_context_t;

typedef struct charness_api_server_context {
    charness_runtime_t *runtime;
    uv_loop_t loop;
    uv_tcp_t server;
} charness_api_server_context_t;

static void close_client(uv_handle_t *handle)
{
    free(handle->data);
}

static void on_write_complete(uv_write_t *request, int status)
{
    (void)status;
    charness_api_client_context_t *client = request->data;
    if (client != NULL) {
        free(client->response);
        client->response = NULL;
        uv_close((uv_handle_t *)&client->handle, close_client);
    }
}

static void send_http_response(charness_api_client_context_t *client, const char *body, int status_code)
{
    const char *status_text = status_code == 200 ? "200 OK" : status_code == 404 ? "404 Not Found" : "500 Internal Server Error";
    char *response = charness_string_format(
        "HTTP/1.1 %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        status_text,
        strlen(body),
        body);

    if (response == NULL) {
        return;
    }

    client->response = response;
    uv_buf_t buffer = uv_buf_init(response, (unsigned int)strlen(response));
    client->write_request.data = client;
    (void)uv_write(&client->write_request, (uv_stream_t *)&client->handle, &buffer, 1, on_write_complete);
}

static char *build_body(charness_runtime_t *runtime, const char *path)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return NULL;
    }

    if (path != NULL && strcmp(path, "/health") == 0) {
        cJSON_AddStringToObject(root, "status", "ok");
        cJSON_AddStringToObject(root, "service", "hive");
        cJSON_AddStringToObject(root, "workspace", safe_text(runtime->session.workspace_root));
        cJSON_AddNumberToObject(root, "iterations", (double)runtime->session.iteration);
    } else {
        cJSON_AddStringToObject(root, "status", "error");
        cJSON_AddStringToObject(root, "message", "unsupported endpoint");
    }

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return body;
}

static void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    charness_api_server_context_t *server = stream->data;
    charness_api_client_context_t *client = stream->handle->data;

    if (nread <= 0) {
        free(buf->base);
        if (nread != UV_EOF) {
            uv_close((uv_handle_t *)stream, close_client);
        } else {
            uv_close((uv_handle_t *)stream, close_client);
        }
        return;
    }

    const char *request = buf->base;
    const char *path = "/unknown";
    if (strncmp(request, "GET /health", 11U) == 0) {
        path = "/health";
    }

    char *body = build_body(server->runtime, path);
    if (body == NULL) {
        body = cJSON_PrintUnformatted(cJSON_CreateObject());
    }

    if (body == NULL) {
        free(buf->base);
        uv_close((uv_handle_t *)stream, close_client);
        return;
    }

    send_http_response(client, body, strcmp(path, "/health") == 0 ? 200 : 404);
    free(body);
    free(buf->base);
}

static void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    (void)handle;
    char *base = malloc(suggested_size);
    buf->base = base;
    buf->len = base != NULL ? suggested_size : 0U;
}

static void on_new_connection(uv_stream_t *server_stream, int status)
{
    charness_api_server_context_t *server = server_stream->data;
    if (status < 0) {
        return;
    }

    charness_api_client_context_t *client = calloc(1U, sizeof(*client));
    if (client == NULL) {
        return;
    }

    client->handle.data = client;
    if (uv_tcp_init(&server->loop, &client->handle) != 0) {
        free(client);
        return;
    }

    if (uv_accept(server_stream, (uv_stream_t *)&client->handle) == 0) {
        (void)uv_read_start((uv_stream_t *)&client->handle, alloc_buffer, on_read);
    } else {
        uv_close((uv_handle_t *)&client->handle, close_client);
    }
}

charness_status_t charness_api_server_run(charness_runtime_t *runtime)
{
    if (runtime == NULL) {
        return CHARNESS_STATUS_INVALID_ARGUMENT;
    }

    charness_api_server_context_t context;
    memset(&context, 0, sizeof(context));
    context.runtime = runtime;

    if (uv_loop_init(&context.loop) != 0) {
        return CHARNESS_STATUS_ERROR;
    }

    if (uv_tcp_init(&context.loop, &context.server) != 0) {
        uv_loop_close(&context.loop);
        return CHARNESS_STATUS_ERROR;
    }

    context.server.data = &context;
    uv_ip4_addr(runtime->options.api_bind_address, (int)runtime->options.api_port, (struct sockaddr_in *)&context.server);
    if (uv_tcp_bind(&context.server, (const struct sockaddr *)&context.server, 0) != 0) {
        uv_loop_close(&context.loop);
        return CHARNESS_STATUS_ERROR;
    }

    if (uv_listen((uv_stream_t *)&context.server, 16, on_new_connection) != 0) {
        uv_loop_close(&context.loop);
        return CHARNESS_STATUS_ERROR;
    }

    if (runtime->logger.initialized) {
        charness_logger_logf(&runtime->logger,
                             CHARNESS_LOG_INFO,
                             "api",
                             "listen",
                             "listening on %s:%u",
                             runtime->options.api_bind_address,
                             runtime->options.api_port);
    }

    (void)uv_run(&context.loop, UV_RUN_DEFAULT);
    uv_loop_close(&context.loop);
    return CHARNESS_STATUS_OK;
}

#else

charness_status_t charness_api_server_run(charness_runtime_t *runtime)
{
    (void)runtime;
    return CHARNESS_STATUS_UNAVAILABLE;
}

#endif
