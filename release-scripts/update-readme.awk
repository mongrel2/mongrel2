BEGIN {
    repo = ENVIRON["REPO"];
    release_name = ENVIRON["RELEASE_NAME"];
    owner = ENVIRON["OWNER"]
    tar_name = repo "-" release_name ".tar.bz2";
}
/tar.bz2/ {
    printf("* [%s](https://github.com/%s/%s/releases/%s/download/%s)\n",tar_name,owner,repo,release_name,tar_name);
    next;
}

/is now [^ ]* as of/ {
    print repo " is now " release_name strftime(" as of *%a %b %d %Y*:",systime());
    next;
}

{ print }
