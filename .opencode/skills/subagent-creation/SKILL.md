---
name: subagent-creation
description: Patterns and best practices for creating specialized subagents in the OpenCode system
---

## Subagent Creation Framework

### Definition of Subagents in OpenCode

In the OpenCode system, subagents are specialized autonomous entities that can be launched via the `task` tool to handle specific types of complex, multi-step operations. Unlike skills which provide static knowledge, subagents are dynamic agents that execute tasks and can make autonomous decisions within their domain.

The primary subagent types available in OpenCode are:
- **general**: General-purpose agent for researching complex questions and executing multi-step tasks
- **explore**: Fast agent specialized for exploring codebases and answering questions about the codebase

### Subagent Creation Patterns

#### 1. Hierarchical Decomposition Pattern
Break complex tasks into smaller, manageable subtasks handled by specialized subagents:
```javascript
task(subagent_type="explore", prompt="Explore the codebase to identify all functions related to temperature control")
task(subagent_type="general", prompt="Implement unit tests for the temperature control functions identified")
```

#### 2. Specialized Functional Pattern
Create subagents for specific functional areas:
- **Code Analysis Agent**: For examining code structure and identifying patterns
- **Research Agent**: For investigating best practices and solutions
- **Implementation Agent**: For writing code based on research findings
- **Testing Agent**: For creating and running tests

#### 3. Domain-Specific Pattern
Develop subagents focused on specific domains within the project:
- **ESP-IDF Agent**: Specialized for ESP-IDF command usage and environment setup
- **Testing Agent**: Focused on unit testing patterns and CMock integration
- **UI Agent**: Specialized for display and user interface functionality

### Best Practices for Subagent Design

#### 1. Clear Objective Definition
Each subagent should have a well-defined purpose and scope:
- Specify the exact task the subagent should accomplish
- Define expected deliverables and outcomes
- Limit scope to prevent over-complexity

#### 2. Appropriate Agent Type Selection
Choose the right agent type for the task:
- Use **explore** for research, codebase exploration, and pattern identification
- Use **general** for complex multi-step tasks requiring reasoning and execution

#### 3. Effective Prompt Engineering
Craft prompts that provide sufficient context while being specific:
- Include relevant project context and constraints
- Specify desired output format
- Reference relevant skills or knowledge bases when applicable

#### 4. Resource Management
Consider the computational resources and time requirements:
- Estimate complexity and processing time
- Plan for potential iterations and refinement
- Monitor subagent execution and results

### Template for Subagent Definition

When creating a new subagent approach, follow this template:

```
## Subagent Name: [Descriptive name]

### Purpose
[Clear statement of what the subagent accomplishes]

### Input Requirements
[What information the subagent needs to start]

### Process Steps
[High-level steps the subagent will perform]

### Expected Output
[What the subagent should produce]

### Agent Type Recommendation
[Which subagent_type to use: explore or general]

### Example Usage
[Concrete example of how to invoke the subagent]
```

### Creating the Actual Subagent File

After defining your subagent using the template above, create an actual JSON file for the subagent in the `.opencode/subagents/` directory. The file should have the following structure:

```json
{
  "name": "[subagent-name]",
  "purpose": "[Purpose statement from template]",
  "input_requirements": [
    "[Input requirement 1]",
    "[Input requirement 2]"
  ],
  "process_steps": [
    "[Process step 1]",
    "[Process step 2]"
  ],
  "expected_output": [
    "[Expected output 1]",
    "[Expected output 2]"
  ],
  "agent_type_recommendation": "[general or explore]",
  "example_usage": "[Example usage from template]"
}
```

Name the file `[subagent-name].json` (e.g., `git_commit_assistant.json`) and place it in the `.opencode/subagents/` directory.

### Coordination Protocols

#### Sequential Execution
For dependent tasks where one subagent's output feeds another:
```javascript
result = task(subagent_type="explore", prompt="Find all temperature sensor functions")
task(subagent_type="general", prompt=`Create tests for these functions: ${result}`)
```

#### Parallel Execution
For independent tasks that can run simultaneously:
```javascript
// Run multiple explore tasks in parallel to research different aspects
code_analysis = task(subagent_type="explore", prompt="Analyze main/temperature_control.c")
test_review = task(subagent_type="explore", prompt="Review existing tests for temperature control")
// Both results can be used together later
```

#### Result Integration
Plan how subagent results will be combined or used:
- Store intermediate results for later use
- Combine findings from multiple subagents
- Validate results before proceeding

### Error Handling and Fallbacks

#### Common Subagent Issues
- **Timeout**: Complex tasks may exceed time limits
- **Scope Creep**: Subagents may attempt overly broad tasks
- **Inconsistent Results**: Different subagents may produce conflicting information

#### Mitigation Strategies
- Define clear stopping conditions for subagents
- Set appropriate complexity bounds
- Validate subagent outputs before use
- Prepare fallback approaches if subagent fails

### Examples of Effective Subagent Usage

#### Example 1: Codebase Exploration Task
```javascript
task(subagent_type="explore", 
     prompt="Explore the ESP32 filament dryer codebase to identify all temperature-related functions and their relationships. Focus on files in main/ and include/ directories.")
```

#### Example 2: Multi-Step Implementation Task
```javascript
// First, analyze current implementation
analysis = task(subagent_type="explore", 
                prompt="Examine the current heater control implementation in main/heater.c and identify potential improvements based on ESP-IDF best practices.")

// Then, implement suggested changes
task(subagent_type="general", 
     prompt=`Implement the improvements suggested in this analysis: ${analysis}. Follow the code style guidelines in AGENTS.md.`)
```

### Common Anti-Patterns to Avoid

#### 1. Over-Delegation
Avoid creating subagents for simple tasks that can be handled directly by the main agent.

#### 2. Circular Dependencies
Ensure subagents don't create circular execution patterns where each waits for the other.

#### 3. Unnecessary Complexity
Don't create multiple subagents when a single subagent or direct action would suffice.

### Subagent Lifecycle Management

#### Initiation
- Clearly define the subagent's goal
- Provide necessary context and constraints
- Select appropriate subagent type

#### Monitoring
- Track subagent progress when possible
- Prepare for potential failures
- Plan for result integration

#### Completion
- Validate subagent outputs
- Integrate results appropriately
- Clean up any temporary artifacts