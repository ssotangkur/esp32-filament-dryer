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

### Finding Model IDs

When specifying a model, you need the exact model ID from OpenCode Zen:

**Fetch available models:**
```bash
# Use webfetch to get the model list
curl https://opencode.ai/zen/v1/models
```

**Common OpenCode Zen models:**
- `kimi-k2.5-free` - Free tier, good for general tasks
- `kimi-k2.5` - Paid tier, higher performance
- `claude-sonnet-4` - Anthropic Claude Sonnet
- `gpt-5.1-codex` - OpenAI GPT with code capabilities

**Tip:** Always fetch the current model list as available models change over time.

### Key Configuration Options

- **description** (required): A brief description of what the agent does
- **mode**: Must be `subagent` for subagents
- **model**: Optional model override (see Finding Model IDs above)
- **tools**: Define which tools the agent can access
- **permission**: Configure granular permissions for specific commands

### Subagent Creation Workflow

Follow this complete workflow when creating a new subagent:

#### Step 1: Define Requirements
- What task will this subagent handle?
- What inputs does it need?
- What tools does it require?
- Should it validate inputs or push back if incomplete?

#### Step 2: Find Model ID
Fetch the model list and select the appropriate model for your use case.

#### Step 3: Create Subagent File
Create `.opencode/agents/<subagent_name>.md` with proper frontmatter and system prompt.

#### Step 4: Write System Prompt
Include in the system prompt:
- Clear role definition
- Required inputs with validation rules
- Step-by-step workflow
- Response format (success/error)
- Usage examples

#### Step 5: Update Documentation
Add the subagent to `AGENTS.md` following the existing format:
```markdown
### @<subagent_name> subagent
<Description>
**Usage:** `@<subagent_name> <example command>`
**Required for:** <When to use>
**Example:** `@<subagent_name> <concrete example>`
```

#### Step 6: Test the Subagent
- Invoke it manually to verify it works
- Test input validation (missing inputs should push back)
- Verify the output format matches expectations

### Input Validation Best Practices

Subagents should validate inputs explicitly:

```markdown
## Required Inputs
Before proceeding, you MUST receive:
1. **input1** - Description of what's needed
2. **input2** - Description of what's needed

## Input Validation
If any input is missing, push back and request it:
- Missing input1: "I need [input1]. Please provide [details]"
- Missing input2: "I need [input2]. Please provide [details]"
```

### Complete Real-World Example

Here's the `url_snapshot` subagent we created:

**File:** `.opencode/agents/url_snapshot.md`
```markdown
---
description: Navigates to a URL using Chrome DevTools MCP, takes a snapshot/screenshot, and performs visual analysis
mode: subagent
model: kimi-k2.5-free
tools:
  bash: false
  read: false
  write: false
  edit: false
  grep: false
  glob: false
  webfetch: false
---
You are a URL Snapshot specialist...
[Full system prompt with validation, workflow, examples]
```

**AGENTS.md entry:**
```markdown
### @url_snapshot subagent
Navigates to a URL using Chrome DevTools MCP...
**Usage:** `@url_snapshot navigate to <url> and <visual processing request>`
**Required for:** Visual testing and analysis...
**Example:** `@url_snapshot navigate to file:///path/to/index.html and tell me what you see`
```

### Invoking Subagents

Subagents can be invoked in two ways:
1. Manually with @ mentions: `@git_commit_assistant please commit these changes`
2. Programmatically by primary agents when they recognize the subagent's expertise matches the current task