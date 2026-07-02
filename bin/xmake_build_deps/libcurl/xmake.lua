add_repositories("liii-repo ../../../xmake")
 
add_requires("libcurl")

target("test")
    add_packages("libcurl")
