
set BOOST_BASE_NAME=boost_1_65_1

set BOOST_ARCHIVE=%BOOST_BASE_NAME%.tar.gz
set BOOST_MARKER_FILE_ARCHIVE_IS_EXTRACTED=%BOOST_BASE_NAME%_is_extracted

cd %1

if exist %BOOST_MARKER_FILE_ARCHIVE_IS_EXTRACTED% goto exit
echo Extracting a boost archive file. This may take several minutes ...
..\win32\tar32\tar32_2\minitar.exe -x %BOOST_ARCHIVE% --display-dialog=0
copy nul %BOOST_MARKER_FILE_ARCHIVE_IS_EXTRACTED%
:exit
