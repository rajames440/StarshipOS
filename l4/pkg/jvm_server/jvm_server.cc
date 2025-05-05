#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Declare main symbol for JVM
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

// Entry
int main(int argc, char **argv)
{
  printf("==> Starting jvm_server\n");

  const char* jargs[] = {
    "-Djava.class.path=rom/bootstrap.jar",
    "org.starship.OSGiManager"
  };

  int ret = JLI_Launch(
    2,                   // argc for JVM
    (char**)jargs,       // argv for JVM
    0, nullptr,          // jargc/jargv
    0, nullptr,          // appclassc/appclassv
    "21-starship",       // fullversion
    "21",                // dotversion
    "java",              // pname
    "java",              // lname
    true,                // javaargs
    false,               // cpwildcard
    false,               // javaw
    false                // ergo_class
  );

  printf("==> JVM exited with code %d\n", ret);
  return ret;
}
