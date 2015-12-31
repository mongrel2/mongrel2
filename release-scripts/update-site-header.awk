BEGIN {
    repo = ENVIRON["REPO"];
    release_name = ENVIRON["RELEASE_NAME"];
    owner = ENVIRON["OWNER"]
    tar_name = repo "-" release_name ".tar.bz2";
    indent = "                                                                        "
}
/Latest tar/ {
    printf("%s<li><a href=\"https://github.com/%s/%s/releases/download/%s/%s\">Latest tar</a></li>\n",
            indent,owner,release_name,tar_name);
    next;
}
{ print }
