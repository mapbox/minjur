# Contributing to minjur

## Releasing

To release a new minjur version:

 - Make sure all tests are passing locally and on travis
 - Update version number in
   - minjur_version.hpp
   - README.md
 - Update CHANGELOG.md
 - Make a git tag: `git tag v${VERSION} -a -m "v${VERSION}" && git push --tags`
 - Go to https://github.com/mapbox/minjur/releases
   and edit the new release. Put "Version x.y.z" in title and
   cut-and-paste entry from CHANGELOG.md.
