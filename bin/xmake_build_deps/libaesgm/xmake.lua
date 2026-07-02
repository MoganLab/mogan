add_repositories("liii-repo ../../../xmake")
 
add_requires("liii-libaesgm")

target("test")
    add_packages("liii-libaesgm")
