VERSION=$1

if [ ${VERSION} = "serial" ] || [ ${VERSION} = "pthreads" ] || [ ${VERSION} = "ompss" ] || [ ${VERSION} = "ompss-hyb" ] || [ ${VERSION} = "omp4" ] || [ ${VERSION} = "omp4-hyb" ]; then 
	cd src
	echo -e "\033[32mCleaning directory\033[m"
	make version=${VERSION} clean
	echo -e "\033[32mCompiling ${VERSION} version\033[m"
	make -j4 version=${VERSION}
	echo -e "\033[32mInstalling ${VERSION} version\033[m"
	make version=${VERSION} install
	echo -e "\033[32mCleaning directory\033[m"
	make version=${VERSION} clean
	cd ..
	echo -e "\033[32mDone!\033[m"
else
	echo -e "\033[31m${VERSION} version not supported!\033[m"
fi
