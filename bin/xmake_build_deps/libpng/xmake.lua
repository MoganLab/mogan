add_repositories("liii-repo ../../../xmake")
 
add_requires("libpng")

target("test")
    add_packages("libpng")
