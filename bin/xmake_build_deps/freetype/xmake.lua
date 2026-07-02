add_repositories("liii-repo ../../../xmake")
 
add_requires("freetype")

target("test")
    add_packages("freetype")
