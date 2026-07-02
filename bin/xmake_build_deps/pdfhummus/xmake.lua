add_repositories("liii-repo ../../../xmake")
 
add_requires("liii-pdfhummus")

target("test")
    add_packages("liii-pdfhummus")
