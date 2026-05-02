#include "DebugWindow.h"
#include <string>

int main(int argc, char** argv) {
    std::string snapshotPath = "debug_snapshot.txt";
    if (argc > 1) {
        snapshotPath = argv[1];
    }

    DebugWindow window(snapshotPath);
    window.initWindow();
    window.run();
    return 0;
}
