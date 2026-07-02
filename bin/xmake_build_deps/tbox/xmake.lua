add_repositories("liii-repo ../../../xmake")
 
add_requires("tbox")

target("test")
    add_packages("tbox")
