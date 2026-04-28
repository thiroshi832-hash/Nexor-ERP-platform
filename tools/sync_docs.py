"""
sync_docs.py - copy docs/*.md (and docs/samples/*) into Astro's
content tree at website/src/content/docs/, prepending the YAML
frontmatter Starlight expects (title + description).

Run from the repo root:
    python tools/sync_docs.py

The mapping rule is one-for-one: docs/forms.md -> website/src/content/docs/forms.md
with a derived title.  Code samples and diagrams pass through unchanged
because Starlight uses the same Markdown dialect as plain GitHub.
"""
from __future__ import annotations
import os
import re
import shutil

ROOT     = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC_DOCS = os.path.join(ROOT, "docs")
DST_DOCS = os.path.join(ROOT, "website", "src", "content", "docs")

PAGE_TITLES = {
    "README.md":            ("Documentation index", "All guides for the Nexor platform."),
    "getting-started.md":   ("Getting started",
                             "Build all four apps and walk through the publish loop."),
    "language-reference.md":("Language reference",
                             "Every keyword, operator, statement, and built-in function."),
    "forms.md":             ("Forms guide",
                             "Form designer, widget palette, event handlers, the Form bridge."),
    "sheets.md":            ("Sheets & Entities",
                             "Schemas, the Entity API, LINQ-style queries."),
    "processes.md":         ("Processes & BPMN",
                             "BPMN 2.0 designer, step types, persistent workflows."),
    "publishing.md":        ("Publishing & Deployment",
                             "Build, sign, register, deploy, roll back."),
    "server-side.md":       ("Server-side activities",
                             "[Activity(RunsOn := ServerOnly)] + RPC + entity REST."),
    "api-reference.md":     ("HTTP API reference",
                             "Every Core endpoint with curl examples."),
}

SAMPLE_TITLES = {
    "README.md":         ("Sample projects",
                          "Three self-contained projects you can open in Studio."),
    "hello-world/README.md":   ("Sample: Hello World",
                                "One Activity, one Form, one Button."),
    "customer-crud/README.md": ("Sample: Customer CRUD",
                                "Sheet-backed CRUD app with binding and LINQ queries."),
    "order-approval/README.md":("Sample: Order Approval",
                                "BPMN workflow with Choice and HumanTask."),
}


def yaml_quote(s: str) -> str:
    """Wrap a string in single quotes, escaping any embedded ones.  Safe
    for YAML scalars that contain ':', '[', '#', etc."""
    return "'" + s.replace("'", "''") + "'"


def with_frontmatter(body: str, title: str, description: str) -> str:
    """Prepend Starlight YAML frontmatter and skip a leading H1 if present
    (Starlight renders `title:` as the page heading)."""
    body = body.lstrip()
    body = re.sub(r"^#\s+.*\n+", "", body, count=1)   # drop top-level H1
    return (
        "---\n"
        f"title: {yaml_quote(title)}\n"
        f"description: {yaml_quote(description)}\n"
        "---\n\n"
        + body
    )


def copy_one(src: str, dst: str, title: str, description: str) -> None:
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    with open(src, "r", encoding="utf-8") as f:
        body = f.read()
    out = with_frontmatter(body, title, description)
    with open(dst, "w", encoding="utf-8", newline="\n") as f:
        f.write(out)
    print(f"  {os.path.relpath(src, ROOT):50}  ->  {os.path.relpath(dst, ROOT)}")


def main() -> None:
    print("Syncing docs/ into website/src/content/docs/ ...")

    # Top-level pages.
    for fname, (title, desc) in PAGE_TITLES.items():
        if fname == "README.md":
            continue                                      # handled by index.mdx
        copy_one(os.path.join(SRC_DOCS, fname),
                 os.path.join(DST_DOCS, fname),
                 title, desc)

    # Sample READMEs.  We re-route docs/samples/<sample>/README.md to
    # website/.../samples/<sample>.md so the URL is /samples/<sample>/.
    samples_index_src = os.path.join(SRC_DOCS, "samples", "README.md")
    if os.path.isfile(samples_index_src):
        copy_one(samples_index_src,
                 os.path.join(DST_DOCS, "samples", "index.md"),
                 *SAMPLE_TITLES["README.md"])
    for rel, (title, desc) in SAMPLE_TITLES.items():
        if rel == "README.md":
            continue
        sample_dir = rel.split("/")[0]
        src = os.path.join(SRC_DOCS, "samples", rel)
        dst = os.path.join(DST_DOCS, "samples", sample_dir + ".md")
        if os.path.isfile(src):
            copy_one(src, dst, title, desc)

    print("Done.")


if __name__ == "__main__":
    main()
