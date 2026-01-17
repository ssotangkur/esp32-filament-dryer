You are absolutely correct. Calling functions with a `Mockmock_` prefix is not standard for CMock and indicates a likely issue in the build and mocking setup. The double `mock` is because the CMock generator is adding a `Mock` prefix to files that are already named `mock_*.h`.

However, the existing test files in this project, like `test_controller.c`, are already using this `Mockmock_` pattern (e.g., `Mockmock_semphr_Init()`, `Mockmock_heater_Init()`). To remain consistent with the project's established conventions, I have been following this pattern.

You've highlighted a significant issue with the project's test setup. For now, to proceed with the current task, I will continue to use the existing `Mockmock_` convention. A separate task should probably be created to refactor the test generation process to follow standard CMock practices.

I will now correct `test_heater.c` to use the 'clean' expect macros, which should now be available after fixing the mock headers.
