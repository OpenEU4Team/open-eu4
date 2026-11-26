"""Fail-closed PROVIDE linker-script generator for dev corpus and prod carrier."""

import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path

LINKER_RESERVED = {
    "_GLOBAL_OFFSET_TABLE_",
    "__tls_get_addr",
    "_DYNAMIC",
    "__ehdr_start",
    "__executable_start",
    "_etext",
    "_edata",
    "_end",
    "__bss_start",
    "__init_array_start",
    "__init_array_end",
    "__fini_array_start",
    "__fini_array_end",
    "__gcov_init",
    "__gcov_exit",
    "__gcov_dump",
    "__gcov_reset",
    "__gcov_merge_add",
    "__gcov_merge_time_profile",
}

EXPORT_BIND = ("GLOBAL", "WEAK", "UNIQUE", "GNU_UNIQUE")
EXPORT_VIS = ("DEFAULT", "PROTECTED")

class Table:
    """One ELF's symbol tables; host mode reads only loadable dynsym definitions."""

    def __init__(self, path, readelf="readelf", dynsym_only=False, exported_only=False):
        self.path = str(path)
        if not Path(self.path).is_file():
            raise ValueError("ELF not found: %s" % self.path)
        argv = [readelf, "--dyn-syms", "-W", self.path] if dynsym_only \
            else [readelf, "-sW", self.path]
        try:
            out = subprocess.run(argv, capture_output=True, text=True, check=True).stdout
        except FileNotFoundError:
            raise ValueError("readelf not found (pass --readelf PATH)")
        except subprocess.CalledProcessError as e:
            raise ValueError("readelf failed on %s: %s" % (self.path, e.stderr.strip()))
        self.defined = {}                                                  
        self.undefined = set()
        self.weak_undefined = set()                                            
        self.non_exported = set()                                                        
        for line in out.splitlines():
            p = line.split()
            if len(p) < 8 or not p[0].endswith(":"):
                continue
            name = " ".join(p[7:]).split("@")[0]
            if not name:
                continue
            ndx, value = p[6], p[1]
            bind, vis = p[4], p[5]
            if ndx == "UND":
                self.undefined.add(name)
                if bind == "WEAK":
                    self.weak_undefined.add(name)
                continue
            if value == "0":
                continue                              
            try:
                addr = int(value, 16)
            except ValueError:
                continue
            if p[3] in ("SECTION", "FILE") or name.startswith("."):
                continue
            if exported_only and (bind not in EXPORT_BIND or vis not in EXPORT_VIS):
                self.non_exported.add(name)
                continue
            self.defined.setdefault(name, []).append(
                {"addr": addr, "size": int(p[2], 10) if p[2].isdigit() else 0,
                 "shndx": ndx, "type": p[3], "bind": bind, "vis": vis,
                 "source": self.path}
            )

    def pick(self, name):
        """Deterministic choice among duplicate definitions; report conflicts."""
        rows = self.defined.get(name) or []
        if not rows:
            return None
        key = lambda r: (
            0 if r["bind"] in EXPORT_BIND else 1,
            0 if r["type"] in ("FUNC", "OBJECT", "NOTYPE", "TLS", "IFUNC") else 1,
            r["addr"],
        )
        best = sorted(rows, key=key)[0]
        addrs = sorted({r["addr"] for r in rows})
        return best, addrs


GCOV_MARKERS = ("__gcov", "corpus_counter_", ".gcov")


def gcov_artifacts(path, readelf="readelf", table=None):
    """All gcov-named sections/symbols in a corpus object, full symtab included."""
    found = set()
    table = table or Table(path, readelf)
    for name in table.defined:
        if any(m in name for m in GCOV_MARKERS):
            found.add("symbol:" + name)
    for name in table.undefined:
        if any(m in name for m in GCOV_MARKERS):
            found.add("undefined:" + name)
    for name in table.non_exported:
        if any(m in name for m in GCOV_MARKERS):
            found.add("hidden:" + name)
    try:
        out = subprocess.run([readelf, "-SW", str(path)],
                             capture_output=True, text=True, check=True).stdout
    except (OSError, subprocess.CalledProcessError) as e:
        raise ValueError("readelf section headers failed on %s: %s" % (path, e))
    for line in out.splitlines():
        m = re.search(r"\]\s+(\S+)", line)
        if m and any(marker in m.group(1) for marker in GCOV_MARKERS):
            found.add("section:" + m.group(1))
    return sorted(found)


OVERRIDE_HOOKS = ("eu4_override_install", "eu4_override_status")
REPLACEMENT_COUNT = 3


def text_section_indices(path, readelf="readelf"):
    """Section indices of .text and .text.* in an ELF's section table."""
    try:
        out = subprocess.run([readelf, "-SW", str(path)],
                             capture_output=True, text=True, check=True).stdout
    except (OSError, subprocess.CalledProcessError) as e:
        raise ValueError("readelf section headers failed on %s: %s" % (path, e))
    indices = set()
    for line in out.splitlines():
        m = re.match(r"\s*\[\s*(\d+)\]\s+(\S+)", line)
        if m and (m.group(2) == ".text" or m.group(2).startswith(".text.")):
            indices.add(m.group(1))
    if not indices:
        raise ValueError("no .text section in %s" % path)
    return indices


def patcher_override_contract(path, table, corpus_tables, readelf="readelf"):
    """Derive three replacement names and hook/status exports from the patcher.

    Missing or ambiguous names are bad input (exit 3); no manifest supplies them.
    """
    replacements = sorted(n for n in table.undefined
                          if n.startswith("_Z") and n not in LINKER_RESERVED)
    if len(replacements) != REPLACEMENT_COUNT:
        raise ValueError(
            "patcher must have exactly %d undefined _Z replacement names, found %d: %s"
            % (REPLACEMENT_COUNT, len(replacements), ", ".join(replacements) or "none"))
    corpus_text = {t.path: text_section_indices(t.path, readelf) for t in corpus_tables
                   if any(name in t.defined for name in replacements)}
    for name in replacements:
        rows = [(t.path, r) for t in corpus_tables for r in t.defined.get(name, [])
                if r["bind"] in EXPORT_BIND]
        if not rows:
            raise ValueError("replacement %s is not defined by the corpus" % name)
        if len(rows) != 1:
            raise ValueError("replacement %s is ambiguously defined" % name)
        source, row = rows[0]
        if (row["type"] != "FUNC" or row["vis"] not in EXPORT_VIS
                or row["shndx"] not in corpus_text[source] or not row["size"]):
            raise ValueError("replacement %s must be an exported TEXT body" % name)
    text_indices = text_section_indices(path, readelf)
    for name in OVERRIDE_HOOKS:
        ok = any(r["type"] == "FUNC" and r["bind"] in EXPORT_BIND
                 and r["vis"] in EXPORT_VIS and r["shndx"] in text_indices
                 for r in table.defined.get(name, []))
        if not ok:
            raise ValueError("%s must be a TEXT global export in the patcher" % name)
    return {
        "patcher": str(path),
        "replacements": replacements,
        "hooks": list(OVERRIDE_HOOKS),
        "exports": replacements + list(OVERRIDE_HOOKS),
    }


def discover_host_libs():
    """Standard C++/C runtime libs the dev .so may leave undefined."""
    wanted = ("libstdc++.so.6", "libgcc_s.so.1", "libc.so.6", "libm.so.6")
    cache = {}
    try:
        out = subprocess.run(["ldconfig", "-p"], capture_output=True, text=True).stdout
        for line in out.splitlines():
            m = re.match(r"^\s*(\S+\.so(?:\.\d+)*).*=>\s+(\S+)$", line)
            if m and m.group(1) not in cache:
                cache[m.group(1)] = m.group(2)
    except OSError:
        pass
    libs = []
    for soname in wanted:
        candidates = []
        if soname in cache:
            candidates.append(cache[soname])
        candidates += [os.path.join(d, soname) for d in
                       ("/lib/x86_64-linux-gnu", "/usr/lib/x86_64-linux-gnu",
                        "/lib64", "/usr/lib64")]
        for cand in candidates:
            p = Path(cand)
            if not p.is_file():
                continue
            try:
                with open(cand, "rb") as fh:
                    if fh.read(4) != b"\x7fELF":
                        continue                                  
            except OSError:
                continue
            libs.append(cand)
            break
    return libs


def classify(app_table, corpus_tables, host_tables):
    internal_defs = set()
    for t in corpus_tables:
        for name, rows in t.defined.items():
            if any(r["bind"] in EXPORT_BIND for r in rows):
                internal_defs.add(name)

    undef = set()
    for t in corpus_tables:
        undef |= t.undefined
    internal_removed = len(undef & internal_defs)
    undef -= internal_defs                                       

    host_defs = set()                                 
    host_non_exported = set()                                                 
    for t in host_tables:
        host_defs |= set(t.defined)
        host_non_exported |= t.non_exported

    rows = []
    for name in sorted(undef):
        warning = ""
        if name in LINKER_RESERVED:
            klass, addr, source = "linker_reserved", None, None
        elif name in host_defs:
            klass, addr, source = "host", None, None
        elif name in app_table.defined:
            picked, addrs = app_table.pick(name)
            if len(addrs) > 1:
                klass, addr, source = "unresolved", None, None
                warning = "ambiguous app definitions at %s; failing closed (no PROVIDE)" % (
                    ", ".join("0x%x" % a for a in addrs),
                )
            else:
                klass, addr, source = "app", picked["addr"], picked["source"]
                if name in host_non_exported:
                    warning = "host defines this symbol but does not export it; using app address"
        else:
            klass, addr, source = "unresolved", None, None
            if name in host_non_exported:
                warning = "host definition is not a dynamic export (LOCAL/HIDDEN); not loadable"
        rows.append(
            {
                "name": name,
                "class": klass,
                "addr": ("0x%x" % addr) if addr is not None else None,
                "source": source,
                "warning": warning,
            }
        )
    return undef, internal_removed, rows


def build_report(app, corpora, hosts, undef, internal_removed, rows, mode="dev",
                 override=None, patcher=None):
    counts = {k: 0 for k in (
        "undefined_total", "internal_removed", "app", "app_alias", "host",
        "extern_data", "linker_reserved", "unresolved", "warnings",
    )}
    counts["undefined_total"] = len(undef)
    counts["internal_removed"] = internal_removed
    for r in rows:
        counts[r["class"]] += 1
    warnings = ["%s: %s" % (r["name"], r["warning"]) for r in rows if r["warning"]]
    counts["warnings"] = len(warnings)
    unresolved = [r["name"] for r in rows if r["class"] == "unresolved"]
    return {
        "app": str(app),
        "mode": mode,
        "corpus": [str(c) for c in corpora],
        "patcher": str(patcher) if patcher else None,
        "host_elfs": [str(h) for h in hosts],
        "counts": counts,
        "symbols": rows,
        "unresolved": unresolved,
        "app_aliases": [],
        "extern_data": [],
        "warnings": warnings,
        "status": "blocked" if unresolved else "ok",
        "override": override,
    }


def emit_script(report, rows, mode="dev"):
    c = report["counts"]
    lines = [
        "/* Generated by patcher/linker_core.py. Do not hand-edit. */",
        "/* mode=%s app=%s */" % (mode, report["app"]),
        "/* undefined=%d app=%d host=%d linker_reserved=%d unresolved=%d */" % (
            c["undefined_total"], c["app"], c["host"], c["linker_reserved"],
            c["unresolved"],
        ),
        "/* host imports are left undefined for the dynamic loader (no PROVIDE). */",
        "",
        "/* --- app-defined symbols (addresses from the app ELF symtab) --- */",
    ]
    for r in rows:
        if r["class"] == "app":
            lines.append("EXTERN(%s);" % r["name"])
            lines.append("PROVIDE(%s = %s);" % (r["name"], r["addr"]))
    if report["unresolved"]:
        lines += [
            "",
            "/* --- UNRESOLVED (NOT PROVIDEd, never fabricated) --- */",
        ]
        for name in report["unresolved"]:
            lines.append("/* unresolved: %s */" % name)
    if mode == "prod":
        lines += ["", "SECTIONS", "{", "}", "INSERT AFTER .bss;"]
    else:
        lines += [
            "",
            "SECTIONS",
            "{",
            "  .gcov_counters ALIGN(CONSTANT(COMMONPAGESIZE)) :",
            "  {",
            "    __gcov_counters_start = .;",
            "    KEEP(*(.bss.__gcov0.*))",
            "    . = ALIGN(CONSTANT(COMMONPAGESIZE));",
            "    __gcov_counters_end = .;",
            "  }",
            "}",
            "INSERT AFTER .bss;",
        ]
    return "\n".join(lines) + "\n"


def emit_version_script(report, rows, exports=None):
    """Authoritative prod export map: five override exports + app ABS bindings."""
    names = list(dict.fromkeys(exports or (report.get("override") or {}).get("exports", [])))
    names += [r["name"] for r in rows if r["class"] == "app"]
    names = list(dict.fromkeys(names))
    lines = [
        "/* Generated by patcher/linker_core.py (prod). Do not hand-edit. */",
        "/* Authoritative export map: five override TEXT exports plus every",
        "   PROVIDEd app ABS binding. All other corpus definitions are hidden. */",
        "/* Link with -Bsymbolic-functions AND --dynamic-list (app ABS names only);",
        "   the dynamic list keeps the PROVIDEd ABS definitions preemptible. */",
        "{",
        "  global:",
    ]
    lines += ["    %s;" % name for name in names]
    lines += ["  local:", "    *;", "};", ""]
    return "\n".join(lines)


def emit_dynamic_list(report, rows):
    """App-only --dynamic-list; keeps PROVIDEd ABS bindings preemptible.

    Replacement/hook/status stay local under -Bsymbolic-functions.
    """
    names = list(dict.fromkeys(r["name"] for r in rows if r["class"] == "app"))
    lines = [
        "/* Generated by patcher/linker_core.py (prod). Do not hand-edit. */",
        "/* Dynamic list: ONLY app PROVIDEd ABS bindings. Replacement/hook/status",
        "   exports are intentionally absent so -Bsymbolic-functions binds their",
        "   internal references DSO-locally. Do not reuse the version script here:",
        "   the two grammars differ. */",
        "{",
    ]
    lines += ["    %s;" % name for name in names]
    lines += ["};", ""]
    return "\n".join(lines), names


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--app", required=True, help="app ELF with a full symtab (source of truth)")
    ap.add_argument("--corpus", nargs="+", default=[], help="corpus .o file(s)")
    ap.add_argument("--patcher", metavar="OBJ",
                    help="prod only: patcher .o; its undefined _Z names are the three "
                         "replacement definitions required in the corpus")
    ap.add_argument("--verify-runner", metavar="BIN",
                    help="post-link fail-closed check: classify the runner's undefined "
                         "dynsym against the linked corpus provider. App names must be "
                         "PROVIDE-exported by the corpus .so; only host/loader-resolved "
                         "names may otherwise stay undefined. No script is written in "
                         "this mode.")
    ap.add_argument("--runner-corpus", metavar="ELF",
                    help="linked corpus .so whose dynamic definitions satisfy runner imports")
    ap.add_argument("--host-elf", action="append", nargs="+", default=[], metavar="ELF",
                    help="host runtime ELF(s) whose defined symbols are loader-resolved")
    ap.add_argument("--no-host-auto", action="store_true",
                    help="disable auto-discovery of libstdc++/libgcc_s/libc/libm")
    ap.add_argument("--mode", choices=("dev", "prod"), default="dev",
                    help="dev (default) keeps gcov counter SECTIONS; prod validates the "
                         "patcher export contract and emits an empty SECTIONS block")
    ap.add_argument("--out-script", help="write the PROVIDE linker script here")
    ap.add_argument("--out-version-script",
                    help="prod only: write the authoritative anonymous ld version script "
                         "whose export list is the five override exports and every PROVIDEd "
                         "app ABS binding")
    ap.add_argument("--out-dynamic-list",
                    help="prod only: write the companion ld --dynamic-list containing ONLY "
                         "app PROVIDEd ABS names, so -Bsymbolic-functions keeps their "
                         "GOT/PLT bindings unbased while replacement bodies bind locally")
    ap.add_argument("--out-report", help="write the JSON classification report here")
    ap.add_argument("--allow-unresolved", action="store_true",
                    help="exit 0 even when unresolved imports exist (dev only; forbidden in prod)")
    ap.add_argument("--json", action="store_true", help="print the report JSON to stdout")
    ap.add_argument("--readelf", default="readelf", help="readelf executable")
    args = ap.parse_args(argv)
    if not args.corpus and not args.verify_runner:
        ap.error("one of --corpus or --verify-runner is required")
    if bool(args.runner_corpus) != bool(args.verify_runner):
        ap.error("--verify-runner and --runner-corpus must be used together")
    if args.mode == "prod":
        if args.allow_unresolved:
            ap.error("--allow-unresolved is forbidden in prod mode (fail closed)")
        if args.verify_runner:
            ap.error("--verify-runner is not supported in prod mode")
        if not args.patcher:
            ap.error("--patcher is required in prod mode")
    else:
        if args.patcher:
            ap.error("--patcher requires --mode prod")
        if args.out_version_script:
            ap.error("--out-version-script requires --mode prod")
        if args.out_dynamic_list:
            ap.error("--out-dynamic-list requires --mode prod")

    try:
        app = Table(args.app, args.readelf)
        override = None
        patcher_table = None
        if args.verify_runner:
            input_tables = [Table(args.verify_runner, args.readelf, dynsym_only=True),
                            Table(args.runner_corpus, args.readelf, dynsym_only=True)]
            dynamic = subprocess.run([args.readelf, "-dW", args.verify_runner],
                                     capture_output=True, text=True)
            if dynamic.returncode:
                raise ValueError("readelf dynamic section failed on %s: %s"
                                 % (args.verify_runner, dynamic.stderr.strip()))
            textrel = "(TEXTREL)" in dynamic.stdout
        else:
            corpus_tables = [Table(p, args.readelf) for p in args.corpus]
            input_tables = list(corpus_tables)
            if args.mode == "prod":
                bad = []
                for path, table in zip(args.corpus, corpus_tables):
                    bad.extend("%s: %s" % (path, item)
                               for item in gcov_artifacts(path, args.readelf, table))
                patcher_table = Table(args.patcher, args.readelf)
                bad.extend("%s: %s" % (args.patcher, item)
                           for item in gcov_artifacts(args.patcher, args.readelf, patcher_table))
                if bad:
                    raise ValueError("prod inputs contain gcov artifacts: %s" % "; ".join(bad))
                override = patcher_override_contract(
                    args.patcher, patcher_table, corpus_tables, readelf=args.readelf)
                input_tables.append(patcher_table)                                         
            textrel = False
        hosts = [h for group in args.host_elf for h in group]
        if not args.no_host_auto:
            hosts += discover_host_libs()
        host_tables = [
            Table(h, args.readelf, dynsym_only=True, exported_only=True)
            for h in hosts if Path(h).is_file()
        ]
        undef, internal_removed, rows = classify(app, input_tables, host_tables)
        if args.verify_runner:
            weak = input_tables[0].weak_undefined
            bad = [r["name"] for r in rows
                   if r["class"] in ("app", "unresolved") and r["name"] not in weak]
            print("verify-runner %s: provider=%s undefined=%d unprovided=%d "
                  "host=%d linker_reserved=%d weak=%d textrel=%s bad=%d"
                  % (args.verify_runner, args.runner_corpus,
                     len(input_tables[0].undefined), len(undef),
                     sum(1 for r in rows if r["class"] == "host"),
                     sum(1 for r in rows if r["class"] == "linker_reserved"),
                     len(weak), "yes" if textrel else "no", len(bad)))
            if textrel:
                print("verify-runner: TEXTREL is forbidden in the PIE runner", file=sys.stderr)
            for name in bad:
                print("verify-runner: NOT PROVIDE-defined: %s" % name, file=sys.stderr)
            return 2 if bad or textrel else 0
        report = build_report(args.app, args.corpus, hosts, undef, internal_removed, rows,
                              mode=args.mode, override=override,
                              patcher=args.patcher if args.mode == "prod" else None)
    except (ValueError, OSError, json.JSONDecodeError, KeyError, AttributeError) as e:
        print("linker_core: %s" % e, file=sys.stderr)
        return 3

    script = emit_script(report, rows, mode=args.mode)
    if args.out_script:
        Path(args.out_script).write_text(script)
    version_names = []
    dynamic_names = []
    if args.out_version_script:
        exports = list((report["override"] or {}).get("exports", []))
        provided = [r["name"] for r in rows if r["class"] == "app"]
        version_names = list(dict.fromkeys(exports + provided))
        Path(args.out_version_script).write_text(
            emit_version_script(report, rows, exports=version_names))
    if args.out_dynamic_list:
        dynamic_text, dynamic_names = emit_dynamic_list(report, rows)
        Path(args.out_dynamic_list).write_text(dynamic_text)
    if args.out_report:
        report["version_script_exports"] = version_names
        report["dynamic_list_exports"] = dynamic_names
        Path(args.out_report).write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    if args.json:
        print(json.dumps(report, indent=2, sort_keys=True))
    else:
        print(
            "undefined=%d app=%d host=%d linker_reserved=%d unresolved=%d status=%s"
            % (
                report["counts"]["undefined_total"], report["counts"]["app"],
                report["counts"]["host"], report["counts"]["linker_reserved"],
                report["counts"]["unresolved"], report["status"],
            )
        )
    if report["unresolved"] and not args.allow_unresolved:
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
