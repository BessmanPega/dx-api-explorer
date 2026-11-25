---
name: sync-html
description: Port recent changes in the C++ DX API Explorer (src/) into the single-file browser version, dx_api_explorer.html. Use when the user says "sync the HTML", "update the HTML version", or similar after changing the C++ code.
---

# Syncing dx_api_explorer.html with the C++ source

`dx_api_explorer.html` is a single-file, vanilla-JS port of the essential flow of
the C++ app. It covers config, login, listing case types, creating a case,
opening the assignment action, rendering the form, submitting, and chaining into
the next assignment. The C++ code is the source of truth. The HTML follows it.

The author deploys the file to the same server as the Pega instance (same origin)
for demos, with the `CONFIG` block filled in.

## Workflow

1. **Find the baseline.** Line 2 of the HTML is
   `<!-- SYNCED-WITH: <commit> ... -->`. Run
   `git log --oneline <commit>..HEAD -- src/` and `git diff <commit>..HEAD -- src/`.
   Also check `git status` / `git diff -- src/` for uncommitted C++ changes, and
   ask whether to include them.
2. **Sort each change** using the map below. Changes to ImGui layout, fonts,
   windows, debug or XRay views, threading, or config-file handling usually have
   no HTML counterpart. Say so and skip them.
3. **Port it.** Keep the HTML's structure and names parallel to the C++ ones, so
   the next sync stays a near-mechanical diff.
4. **If a change touches network calls, read the cpp-httplib code it relies on**
   (`lib/cpp-httplib/httplib.h`, v0.16.0). Don't assume what an endpoint "ought"
   to accept. See "Wire fidelity" below.
5. **Smoke-test** with the mock server (see "Verifying"). Extend its canned
   responses if the change adds a component or call it doesn't cover.
6. **Update the SYNCED-WITH commit** to the C++ commit you ported up to (use
   `HEAD`'s short hash if everything is committed). Then report what you ported,
   what you skipped and why, and anything that couldn't be verified.

## C++ to HTML map

| C++ (src/) | HTML |
|---|---|
| `dx_api_network_procs.h` `handle_request` cases | `requestToken()`, `dx(...)` call sites in `go`, `loadCaseTypes`, `createCase`, `openAssignmentAction`, `submitAction` |
| `handle_response` cases | the code after each `await dx(...)` / `requestToken()` |
| `submit_open_assignment_action` | `submitAction` (content collection) |
| `dx_api_model_procs.h` `parse_dx_response`, `resolve_label`, `resolve_name`, `get_content` | `parseDxResponse`, `resolveLabel`, `resolveName`, `getContent`, `contentToString` |
| `dx_api_model_types.h` `component_type_t` / strings | `COMPONENT_TYPES`, `FIELD_TYPES` |
| `dx_api_component_type_procs.cpp` `make_component_r` | `makeComponent` |
| `validate_component_r` | `validateComponent` |
| `draw_component_r` | `renderComponent` |
| `dx_api_helper_procs.h` `to_bool`, `to_enum` | `toBool`, `toComponentType` |
| `dx_api_draw_procs.h` login form / case type menu / action Submit | config `<details>`, `loadCaseTypes` buttons, `openAssignmentAction` |

When a new component type is added in C++, it usually touches all of
`COMPONENT_TYPES`/`FIELD_TYPES`, `makeComponent`, `validateComponent`, and
`renderComponent`.

## Wire fidelity (don't regress these)

Requests must match what cpp-httplib actually sends, not what seems "correct":

- **Path encoding:** every path goes through `encodeUrl()`, a port of httplib's
  `detail::encode_url`. It encodes only space, `+`, CR, LF, `'`, `,`, `;` and
  non-ASCII bytes. Never use `encodeURIComponent`. `!` in assignment IDs goes out raw.
- **Login:** Basic auth over the UTF-8 bytes of `clientId:clientSecret`,
  `Accept: application/json`, `Content-Type: application/x-www-form-urlencoded`,
  and the body built by plain string concatenation, with no extra encoding.
  This is working code. Don't "fix" it.
- **DX calls:** `Authorization: Bearer`, and `Accept: */*` (httplib's default).
  POST and PATCH always send `Content-Type: application/json`, even with no
  body (httplib then sends `Content-Length: 0`).
- **fetch options mirror httplib:** `credentials: "omit"` (no cookies),
  `redirect: "manual"` (`follow_location_` is false), `cache: "no-store"`.
- **JSON bodies:** `dumpJson()` uses 2-space indent and sorted keys, like
  nlohmann `dump(2)` with its `std::map`-backed objects. Iterating
  `resources.fields`/`views` also uses sorted key order for the same reason.
- **Submitted values are strings,** taken from the flattened content
  (`contentToString` mirrors the C++ flattening, including bool -> "1"/"0" and
  `%f`-then-trim for floats).
- **Every editable field that's displayed is submitted.** `draw_component_r`
  sets `is_dirty` when drawing, and `renderComponent` does the same.

General rule: port C++ behaviour as it is, including quirks. If something looks
like a bug, point it out to the user. Don't silently change it in the HTML.

## Intentional differences from the C++ app (keep unless told otherwise)

- After creating a case, it opens the first `canPerform` assignment and its first
  action automatically, skipping the plain `GET /assignments/{id}` and the user's
  picks.
- After a submit, it chains into the next performable assignment the same way.
  Otherwise it shows a confirmation saying why it stopped.
- AutoComplete renders as a plain text input (the C++ dropdown is still a stub).
- There's no debug window, XRay, inspectors, logout, or config-file handling.
  Traffic goes to `console.debug` with secrets masked.
- **All errors go to the browser console only.** Nothing error-related is shown
  on the page. Unsupported components go to `console.warn` and aren't rendered.
- `CONFIG` stays the first script block in `<head>`. If every value is set, the
  page auto-logs in with the form collapsed. Otherwise the form opens.
- Vanilla HTML/JS, one file, no styling, no libraries.

## Verifying

`mock_pega.py` (next to this file) serves the repo's `dx_api_explorer.html` with
`CONFIG` filled in. It plays a two-step case (login, casetypes, create, action 1,
submit, action 2, submit, resolved) and prints every raw request to stdout.

1. Run it in the background: `python -I .claude/skills/sync-html/mock_pega.py`
   (default port 8765).
2. Open `http://localhost:8765/` in the browser pane. Click the case type and
   Submit through both steps (or drive it with JS), and check the console for errors.
3. Read the server's output to confirm the request lines, headers and bodies
   follow the "Wire fidelity" rules. In particular check the `%20`/`!` path
   encoding, the `If-Match` ETag per step, and the verbatim login body.
4. Stop the server.

The mock only proves the page matches the mock. Tell the user that a check
against a real Pega instance is still theirs to do.
