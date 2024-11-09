// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/vespalib/testkit/test_kit.h>
#include <vespa/vespalib/process/process.h>
#include <sys/wait.h>

using namespace vespalib;

TEST_MAIN() {

    ASSERT_EQUAL(argc, 3);

    int retval = strtol(argv[1], NULL, 0);

    fprintf(stderr, "argc=%d : Running '%s' expecting signal %d\n", argc, argv[2], retval);

    Process cmd(argv[2]);
    for (std::string line = cmd.read_line(); !(line.empty() && cmd.eof()); line = cmd.read_line()) {
        fprintf(stdout, "%s\n", line.c_str());
    }
    int exitCode = cmd.join();

    if (exitCode == 65535) {
        fprintf(stderr, "[ERROR] child killed (timeout)\n");
    } else if (WIFEXITED(exitCode)) {
        fprintf(stderr, "child terminated normally with exit code %u\n", WEXITSTATUS(exitCode));
    } else if (WIFSIGNALED(exitCode)) {
        fprintf(stderr, "child terminated by signal %u\n", WTERMSIG(exitCode));
        if (WCOREDUMP(exitCode)) {
            fprintf(stderr, "[WARNING] child dumped core\n");
        }
    } else {
        fprintf(stderr, "[WARNING] strange exit code: %u\n", exitCode);
    }

    EXPECT_EQUAL(exitCode & 0x7f, retval);
}
