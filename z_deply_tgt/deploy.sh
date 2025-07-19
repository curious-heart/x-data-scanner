#!/bin/sh

DEPLOY_APP=/media/kylin/49487059-c39d-4e34-b26a-a2bd363fee4b/16.back-scattering/00.env/linuxdeployqt-continuous-aarch64.AppImage

export PATH=/media/kylin/49487059-c39d-4e34-b26a-a2bd363fee4b/16.back-scattering/00.env/qt5.15.2-src/qt-everywhere-src-5.15.2/qtbase/bin:$PATH
export LD_LIBRARY_PATH=/media/kylin/49487059-c39d-4e34-b26a-a2bd363fee4b/16.back-scattering/00.env/qt5.15.2-src/qt-everywhere-src-5.15.2/qtbase/lib:$LD_LIBRARY_PATH

$DEPLOY_APP $1 -appimage -no-copy-copyright-files

#now create the run.sh
pth=$(dirname "$1")
app_name=$(basename "$1")
run_sh_fpn="$pth/run.sh"

cat > $run_sh_fpn << EOF
#!/bin/sh
export QT_XCB_GL_INTEGRATION=none
EOF

echo "./$app_name" >> $run_sh_fpn

chmod +x $run_sh_fpn
