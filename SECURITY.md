# Security Policy

The Autonomous 2D Organism Simulation project takes security and memory safety seriously. We appreciate the responsible disclosure of any vulnerabilities found within this codebase.

---

## Supported Versions

Security patches and vulnerability evaluations are actively maintained for the latest release and the current state of the default branch:

| Version | Supported          |
| :---    | :---               |
| `main`  | :white_check_mark: |
| < 1.0.0 | :white_check_mark: |

---

## Reporting a Vulnerability

**Please do not report security vulnerabilities through public GitHub issues, discussions, or pull requests.**

If you believe you have discovered a security vulnerability or critical memory-safety flaw in this project:

1. **Email the Maintainer Directly:**  
   Send an email to **[tpsapp@hotmail.com](mailto:tpsapp@hotmail.com)** with the subject line formatted as:  
   `[SECURITY VULNERABILITY] <Brief Description>`

2. **Submission Information & Details:**  
   To help us triage and investigate efficiently, please include:
   - **Vulnerability Description:** A clear explanation of the behavior and potential security or stability impact (e.g., out-of-bounds memory write, buffer overflow, uncontrolled allocation, denial of service).
   - **Affected Component:** Specific module or source file (e.g., `sim_core`, `GridMatrix`, `EntityManager`, `AIEngine`, asset parser, or window setup).
   - **Step-by-Step Reproduction:** Minimal reproduction steps or a self-contained test case.
   - **Proof of Concept (PoC):** Sample input, memory sanitizer trace (AddressSanitizer / UndefinedBehaviorSanitizer output), or exploit harness if available.
   - **Environment Information:** Operating system, architecture, compiler version, CMake version, and any custom build flags.
   - **Suggested Fix / Remediation:** If you have developed a patch or mitigation, please attach it as a unified diff or patch file.

---

## Response & Remediation Timeline

We adhere to industry-standard coordinated vulnerability disclosure practices:

- **Initial Acknowledgment:** Within **48 hours** of receiving your report, you will receive an acknowledgment confirming receipt.
- **Initial Triage & Assessment:** Within **5 to 7 business days**, we will investigate, reproduce the issue, determine its severity, and provide an initial assessment.
- **Remediation & Patch Development:** We will work on a surgical fix and keep you informed with progress updates at least once per week until resolved.
- **Coordinated Disclosure:** We observe a **90-day coordinated disclosure policy** (or a mutually agreed timeline). Once a patch is validated and ready for release, we will coordinate public announcement and credit you appropriately (unless you wish to remain anonymous).

---

## Scope & Non-Qualifying Issues

The following scenarios are considered out of scope:
- Vulnerabilities in third-party dependencies (such as upstream Raylib, X11, or GLFW) unless directly exploitable through our application API or usage pattern. (Please report upstream issues to their respective projects).
- Issues requiring physical hardware tampering or root-level host system compromise.
- Theoretical vulnerabilities without demonstrable impact or memory sanitizer traces.

---

## Security Best Practices for Contributors

Contributors should ensure that:
- Core simulation code (`sim_core`) remains 100% headless, memory-safe, and independent of untrusted exterior inputs.
- All code compiles cleanly with `-fsanitize=address,undefined` and passes the full automated test suite without memory leaks or boundary violations.
