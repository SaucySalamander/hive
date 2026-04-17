# Inference backend abstraction — Starter code and examples

This file contains minimal starter snippets to help implement the core API and a mock adapter for testing.

Minimal `inference.h` (excerpt)
```c
/* minimal excerpt; place a full header under src/api or core/ */
#pragma once
#include <stddef.h>
typedef struct inference_ctx inference_ctx_t;
typedef struct { const char *role; const char *content; } inf_message_t;
typedef struct { const char *model; const inf_message_t *messages; size_t n_messages; double temperature; int max_tokens; int stream; } inf_request_t;
typedef void (*inf_stream_cb_t)(const char *chunk, size_t len, void *user);
inference_ctx_t *inf_create(const char *backend_name, const char *config_json);
void inf_destroy(inference_ctx_t *ctx);
int inf_complete_sync(inference_ctx_t *ctx, const inf_request_t *req, char **out_text);
int inf_complete_stream(inference_ctx_t *ctx, const inf_request_t *req, inf_stream_cb_t cb, void *user, int *cancel_token_out);
void inf_free(char *ptr);
```

Mock adapter (concept)

```c
/* mock_adapter.c — compile as part of tests; returns deterministic response */
#include <stdlib.h>
#include <string.h>
#include "inference.h" /* your project header */

static int mock_init(const char *config_json, void **handle) { (void)config_json; *handle = NULL; return 0; }
static void mock_shutdown(void *handle) { (void)handle; }
static int mock_complete_sync(void *handle, const char *req_json, char **resp_json) {
  (void)handle; (void)req_json;
  const char *r = "{\"text\":\"mock reply\"}";
  *resp_json = strdup(r);
  return 0;
}

/* register the adapter with the core during init */
/* inf_register_adapter(&mock_vtable); */
```

Example usage (pseudo)

```c
inference_ctx_t *ctx = inf_create("mock", "{}");
inf_request_t req = { .model = "hive:mock", .messages = messages, .n_messages = n, .temperature = 0.0, .max_tokens = 256 };
char *text = NULL;
if (inf_complete_sync(ctx, &req, &text) == 0) { puts(text); inf_free(text); }
inf_destroy(ctx);
```

Where to put code
- A thin core implementation can live under `src/core/` or `src/api/` and expose `inference.h` for the rest of Hive. Adapters may live under `src/inference/adapters/` or as independent repos.
