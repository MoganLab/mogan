add_repositories("liii-repo ../../../xmake")
 
add_requires("mupdf")

target("test")
    add_packages("mupdf")
