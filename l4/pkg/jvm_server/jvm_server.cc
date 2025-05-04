#include <l4/re/env>
#include <l4/re/parent>
#include <l4/re/util/program>
#include <l4/sys/debug.h>

using namespace L4Re;
using namespace L4Re::Util;

int launch_jvm(const char* jar, const char* main_class)
{
    const char* java_bin = "/rom/java/bin/java";

    const char* argv[] = {
        "java",
        "-cp", jar,
        main_class,
        nullptr
      };

    Cap<Task> task = Env::env()->parent()->create_task();
    if (!task.is_valid()) {
        L4::cout << "❌ Failed to create JVM task.\n";
        return 1;
    }

    Program prog;
    if (prog.load(java_bin, task) < 0) {
        L4::cout << "❌ Failed to load JVM ELF.\n";
        return 2;
    }

    if (prog.run(argv) < 0) {
        L4::cout << "❌ Failed to start JVM.\n";
        return 3;
    }

    L4::cout << "✅ JVM started: java -cp " << jar << " " << main_class << "\n";
    return 0;
}

int main()
{
    L4::cout << "🛸 jvm_server: initializing...\n";
    return launch_jvm("/rom/bootstrap.jar", "org.starship.OSGiManager");
}
