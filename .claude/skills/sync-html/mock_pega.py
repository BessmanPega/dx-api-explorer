"""Tiny mock Pega DX API server for smoke-testing dx_api_explorer.html.

Serves the repo's dx_api_explorer.html at http://localhost:<port>/ with CONFIG
filled in (so the page auto-logs in), answers the DX API calls with canned
responses, and prints every request it receives (request line, headers, body)
to stdout, which shows what is actually on the wire.

Scripted flow: login -> casetypes ("Widget") -> create case -> open action
(step 1) -> submit -> open action (step 2) -> submit -> case resolved.

Usage:  python -I mock_pega.py [port]      (default port 8765)
Extend the canned responses below when testing newly ported components.
"""
import json
import re
import sys
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 8765
HTML = Path(__file__).resolve().parents[3] / "dx_api_explorer.html"

CONFIG = {
    "server": f"http://localhost:{PORT}",
    "dxApiPath": "/prweb/api/application/v2",
    "tokenEndpoint": "/prweb/PRRestService/oauth2/v1/token",
    "clientId": "cid",
    "clientSecret": "sec",
    "userId": "user@example",
    "password": "p+w d",  # '+' and ' ' check that the login body is sent verbatim.
}

CLASS_ID = "O-W"


def assignment(step):
    return {"ID": f"ASSIGN-WORKLIST {CLASS_ID} C-1!STEP{step}", "name": f"Step {step}",
            "canPerform": "true", "actions": [{"ID": f"Act{step}", "name": f"Action {step}", "type": "FlowAction"}]}


def case_info(status="New", assignments=None):
    return {"ID": f"{CLASS_ID} C-1", "businessID": "C-1", "name": "Widget", "status": status,
            "caseTypeID": CLASS_ID, "caseTypeName": "Widget",
            "assignments": assignments if assignments is not None else [assignment(1)],
            "content": {"classID": CLASS_ID, "Name": "Bob", "Amount": 12.5, "Notes": "", "pyID": "C-1"}}


UI_RESOURCES = {
    "root": {"type": "reference", "config": {"type": "view", "name": "pyEmbedAssignment", "context": "caseInfo.content"}},
    "resources": {
        "fields": {
            "Name": [{"classID": CLASS_ID, "type": "Text", "label": "Name"}],
            "Amount": [{"classID": CLASS_ID, "type": "Decimal", "label": "Amount"}],
            "Notes": [{"classID": CLASS_ID, "type": "Text", "label": "Notes"}],
            "pyID": [{"classID": CLASS_ID, "type": "Text", "label": "Case ID", "isSpecial": True}],
        },
        "views": {
            "pyEmbedAssignment": [{"type": "View", "name": "pyEmbedAssignment", "classID": CLASS_ID,
                                   "config": {"template": "DefaultForm"}, "children": [
                {"type": "Region", "name": "A", "children": [{"type": "Reference", "config": {"name": "Form", "type": "view"}}]}]}],
            "Form": [{"type": "View", "name": "Form", "classID": CLASS_ID, "config": {"template": "DefaultForm"}, "children": [
                {"type": "Region", "name": "B", "children": [
                    {"type": "TextInput", "config": {"value": "@P .Name", "label": "@FL .Name", "required": True}},
                    {"type": "Currency", "config": {"value": "@P .Amount", "label": "@L Amount"}},
                    {"type": "TextArea", "config": {"value": "@P .Notes", "label": "@L Notes"}},
                    {"type": "TextInput", "config": {"value": "@P .pyID", "label": "@FL .pyID"}},
                    {"type": "Checkbox", "config": {"value": "@P .X", "label": "@L X"}},  # unsupported -> console.warn
                ]}]}],
        },
    },
}


def page():
    html = HTML.read_text(encoding="utf-8")
    config_js = "const CONFIG = " + json.dumps(CONFIG, indent=4) + ";"
    html, n = re.subn(r"const CONFIG = \{[\s\S]*?\n\};", lambda _: config_js, html, count=1)
    if n != 1:
        raise RuntimeError("Could not find the CONFIG block in dx_api_explorer.html")
    return html.encode("utf-8")


class Handler(BaseHTTPRequestHandler):
    submits = 0

    def log_wire(self):
        n = int(self.headers.get("Content-Length") or 0)
        body = self.rfile.read(n).decode("utf-8", "replace") if n else ""
        print(f"{self.requestline}\n{self.headers}{body}\n" + "-" * 60, flush=True)

    def reply(self, payload, ctype="application/json", headers=None):
        data = payload if isinstance(payload, bytes) else json.dumps(payload).encode()
        self.send_response(200)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(data)))
        for k, v in (headers or {}).items():
            self.send_header(k, v)
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        if self.path == "/":
            Handler.submits = 0  # Reloading the page restarts the script.
            return self.reply(page(), "text/html; charset=utf-8")
        if self.path == "/favicon.ico":
            self.send_response(404); self.end_headers(); return
        self.log_wire()
        if self.path.endswith("/casetypes"):
            return self.reply({"applicationIsConstellationCompatible": True,
                               "caseTypes": [{"ID": CLASS_ID, "name": "Widget"}]})
        if "/assignments/" in self.path and "/actions/" in self.path:
            step = 2 if "STEP2" in self.path else 1
            return self.reply({"data": {"caseInfo": case_info(assignments=[assignment(step)])}, "uiResources": UI_RESOURCES},
                              headers={"ETag": f'"etag-step{step}"'})
        self.send_response(404); self.end_headers()

    def do_POST(self):
        self.log_wire()
        if self.path == CONFIG["tokenEndpoint"]:
            return self.reply({"access_token": "TOKEN123", "token_type": "bearer"})
        if self.path.endswith("/cases"):
            return self.reply({"data": {"caseInfo": case_info()}})
        self.send_response(404); self.end_headers()

    def do_PATCH(self):
        self.log_wire()
        Handler.submits += 1
        if Handler.submits == 1:
            return self.reply({"data": {"caseInfo": case_info("Open", [assignment(2)])}})
        return self.reply({"data": {"caseInfo": case_info("Resolved-Completed", [])}})

    def log_message(self, *args):
        pass


if __name__ == "__main__":
    print(f"Mock Pega on http://localhost:{PORT}/  (serving {HTML})", flush=True)
    HTTPServer(("localhost", PORT), Handler).serve_forever()
