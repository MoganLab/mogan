add_repositories("liii-repo ../../../xmake")
 
add_requires("lolly")

target("test")
    add_packages("lolly")
