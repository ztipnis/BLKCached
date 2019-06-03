#!/bin/bash

echo 'Setting up the script...'
# Exit with nonzero exit code if anything fails
set -e
cd build/documentation
git config --global push.default simple
# Pretend to be an user called Travis CI.
git config user.name "Travis CI"
git config user.email "travis@travis-ci.org"
GH_REPO_ORG=`echo $TRAVIS_REPO_SLUG | cut -d "/" -f 1`
GH_REPO_NAME=`echo $TRAVIS_REPO_SLUG | cut -d "/" -f 2`
GH_REPO_REF="github.com/$GH_REPO_ORG/$GH_REPO_NAME.git"


if [ -d "html" ] && [ -f "html/index.html" ]; then
    echo 'Uploading documentation to the gh-pages branch...'
    git fetch origin gh-pages:gh-pages
    git checkout gh-pages
    ls
    rsync -av html/ ../../html/
   	mv html ../../
   	cd ../../
   	git add --all html
    git commit -m "Deploy code docs to GitHub Pages Travis build: ${TRAVIS_BUILD_NUMBER}" -m "Commit: ${TRAVIS_COMMIT}"
    git push --set-upstream --force "git@github.com:ztipnis/BLKCached.git" > /dev/null 2>&1
else
    echo '' >&2
    echo 'Warning: No documentation (html) files have been found!' >&2
    echo 'Warning: Not going to push the documentation to GitHub!' >&2
    exit 1
fi 
