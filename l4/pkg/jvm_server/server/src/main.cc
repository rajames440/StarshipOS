#include <stdio.h>
#include <string.h>
#include <unistd.h>

// HotSpot's internal launcher entry point
extern "C" int JLI_Launch(int argc, char **argv,
                          int jargc, const char** jargv,
                          int appclassc, const char** appclassv,
                          const char* fullversion,
                          const char* dotversion,
                          const char* pname,
                          const char* lname,
                          jboolean javaargs,
                          jboolean cpwildcard,
                          jboolean javaw,
                          jboolean ergo_class);

int main(int argc, char **argv)
{
  printf("==> jvm_server: launching JVM for `java --version`\n");

  char *java_args[] = {
    (char *)"java",
    (char *)"--version",
    nullptr
  };

  // Note: argc = 2, argv = java_args
  int ret = JLI_Launch(
    2,                   // argc
    java_args,           // argv (same as real command-line)
    0, nullptr,          // jargc, jargv (Java-style args)
    0, nullptr,          // appclassc, appclassv (for -jar or class launch)
    "21-starship",       // fullversion (custom printable string)
    "21",                // dotversion
    "java",              // pname
    "java",              // lname
    false,               // javaargs (treat arguments as JVM args, not main class)
    false,               // cpwildcard
    false,               // javaw
    true                 // ergo_class (use class-based ergonomic options)
  );

  printf("==> JVM exited with code %d\n", ret);
  return ret;
}
