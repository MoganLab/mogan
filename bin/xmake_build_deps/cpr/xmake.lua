add_repositories("liii-repo ../../../xmake")
 
add_requires("cpr")

target("test")
    add_packages("cpr")
