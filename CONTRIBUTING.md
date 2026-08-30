# Contributing to PRIoTP

Thank you for your interest in contributing to PRIoTP. We welcome bug reports, documentation improvements, tests, and protocol implementation enhancements.

## Ways to Contribute

- Report bugs and unexpected behavior
- Improve documentation in `README.md`
- Add or improve tests in `PRTP/tests/`
- Improve protocol modules in `PRTP/src/` and sample apps in `PRTP/application/`
- Suggest performance, congestion-control, or reliability improvements

## Getting Started

1. Fork the repository and clone your fork.
2. Create a feature branch:

```bash
git checkout -b feature/short-description
```

3. Build the project from the /PRTP/ directory:

```bash
./configure
make
```

4. Run tests before opening a pull request:

```bash
make check
```

## Development Guidelines

- Keep changes focused and small where possible.
- Follow existing C code style and naming in `PRTP/src/`.
- Update related docs when behavior or interfaces change.
- Include or update tests for functional changes.
- Avoid committing generated files, logs, or local paths.

## Commit Message Suggestions

Use clear, descriptive commit messages. Example formats:

- `fix: handle fragment buffer timeout edge case`
- `docs: clarify setup steps for test environment`
- `test: add regression test for client reconnect`

## Pull Request Checklist

Before submitting a pull request, ensure:

- The code builds successfully (`make`)
- Relevant tests pass (`make check`)
- Documentation is updated (if needed)
- The change is scoped and explained clearly in the PR description

In the pull request description, include:

- What changed
- Why it changed
- How it was tested
- Any known limitations or follow-up work

## Reporting Issues

When opening an issue, include:

- A clear title and concise summary
- Steps to reproduce
- Expected behavior vs actual behavior
- Environment details (OS, compiler version, configuration)
- Relevant logs or error output

## Code of Conduct

Please be respectful and constructive in all project interactions.

## Questions

If something is unclear, open an issue with the `question` label (or mention that the issue is a question) and we will help.
