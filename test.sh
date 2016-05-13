#BENCHMARKS='blackscholes bodytrack canneal dedup facesim ferret fluidanimate freqmine streamcluster swaptions x264'
BENCHMARKS=$2

VERSIONS='serial pthreads omp3 ompss ompss_instr omp4'


input=simlarge
ncores=2

ACTION=$1

case $ACTION in 

"compile")	echo "COMPILATION..."
			echo "============================= COMPLATION LOG =============================" > compile_log.err 
			for bench in ${BENCHMARKS}; do
				echo -e "\n\t${bench}"
				for version in ${VERSIONS}; do
					echo "============================= ${bench}-${version} =============================" >> compile_log.err 
					status=$(./build.sh ${bench} ${version} 2>> compile_log.err)
					echo "=================================== Done ======================================" >> compile_log.err
					
					if (echo $status | grep -q -E "Compilation Failed!|Installation Failed!"); then
						printf '\t\t%10s\t\033[31mFAILED!\033[m\n' "${version}"
					elif (echo $status | grep -q "version not supported!"); then
						printf '\t\t%10s\t\033[33mNOT SUPPORTED!\033[m\n' "${version}"
					else
						printf '\t\t%10s\t\033[32mPASSED!\033[m\n' "${version}"
					fi
				done
			done
			;;

"run")	echo "RUNNING..."
		echo "============================= EXECUTION LOG =============================" > exec_log.err 
		echo "" > exec_log.err
		for bench in ${BENCHMARKS}; do
		
			echo -e "\n\t${bench}"
			#untar the correct input archive
			cd ${ROOT}/${bench}/inputs 2>> exec_log.err
			tar xvf input_${input}.tar > /dev/null 2>> exec_log.err 
			cd ${ROOT}
		
			for version in ${VERSIONS}; do
				echo "============================= ${bench}-${version} =============================" >> exec_log.err 
				
				if ! (${ROOT}/${bench}/bench/run.sh ${version} ${input} ${ncores} 1>> exec_log.err 2>> exec_log.err); then
					printf '\t\t%10s\t\033[31mFAILED!\033[m\n' "${version}"
				else
					printf '\t\t%10s\t\033[32mPASSED!\033[m\n' "${version}"
				fi
				
				echo "==================================== Done =====================================" >> exec_log.err
			done
		done
		;;

*)	echo "Nothing to be done."

esac
