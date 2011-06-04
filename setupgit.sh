git config push.default tracking
git config branch.autosetuprebase always

which git-flow 2>/dev/null
has_it=$?
if [ ${has_it} -gt 0 ];then
  echo "************************************"
  echo "You need gitflow to hack on mongrel2"
  echo "Please see: https://github.com/nvie/gitflow"
  exit 1
fi

echo "Configuring git flow for this repository..."
git flow init -d
