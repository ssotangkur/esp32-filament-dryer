---
name: subagent-creation
description: How to create and configure subagents in OpenCode using the proper markdown format
---

## Subagent Creation in OpenCode

### Understanding Subagents

Subagents in OpenCode are specialized assistants that primary agents can invoke for specific tasks. They can be invoked:
- Automatically by primary agents for specialized tasks based on their descriptions
- Manually by @ mentioning a subagent in your message (e.g., @general help me search for this function)

OpenCode comes with two built-in subagents:
- **General**: A general-purpose agent for researching complex questions and executing multi-step tasks. Has full tool access (except todo), so it can make file changes when needed.
- **Explore**: A fast, read-only agent for exploring codebases. Cannot modify files.

### Creating Custom Subagents

To create a new subagent, create a markdown file in `.opencode/agents/` with the following format:

```markdown
---
description: Brief description of what the agent does
mode: subagent
model: optional model override (e.g., anthropic/claude-3-5-sonnet-20241022)
tools:
  bash: true/false
  read: true/false
  write: true/false
  edit: true/false
  grep: true/false
  glob: true/false
  webfetch: true/false
permission:
  bash:
    "git*": allow    # Allow git commands
    "*": ask        # Ask for approval for all other bash commands
---
System prompt that defines the agent's behavior and responsibilities.
```

### Example Subagent Configuration

To create a git commit assistant subagent, create `.opencode/agents/git_commit_assistant.md`:

```markdown
---
description: Automates the git commit process by analyzing changes, generating appropriate commit messages following project conventions, and executing the commit with proper validation.
mode: subagent
model: anthropic/claude-3-5-sonnet-20241022
tools:
  bash: true
  read: true
  grep: true
permission:
  bash:
    "git*": allow
    "*": ask
---
You are a git commit specialist. Your role is to automate the git commit process by:

1. Running `git status` to identify staged files and modified files
2. If staged files exist AND no modified files have the same names → commit only staged files
3. If any file appears as both staged and modified → prompt parent agent for user decision
4. If no staged files exist → proceed with committing all changes
5. Analyze changes in selected files to understand the nature of modifications
6. Generate hierarchical commit message with intent and specific changes
7. Validate commit message and execute commit with proper error handling
```

### Key Configuration Options

- **description** (required): A brief description of what the agent does
- **mode**: Must be `subagent` for subagents
- **model**: Optional model override
- **tools**: Define which tools the agent can access
- **permission**: Configure granular permissions for specific commands

### Invoking Subagents

Subagents can be invoked in two ways:
1. Manually with @ mentions: `@git_commit_assistant please commit these changes`
2. Programmatically by primary agents when they recognize the subagent's expertise matches the current task