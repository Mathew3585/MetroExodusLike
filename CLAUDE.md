# MetroExodusLike

Solo UE 5.8 vertical slice of a Metro Exodus-like FPS. The user speaks French: answer in French.

- `ROADMAP.md`: the plan, one step at a time (code → compile → test → validate → commit).
- `CONVENTIONS.md`: mandatory project rules (C++ technical base + data Blueprints, naming, folders, design rules).

Key rule: gameplay logic in C++ only; Blueprints/WBP hold data, content and layout. Never `ConstructorHelpers`.
