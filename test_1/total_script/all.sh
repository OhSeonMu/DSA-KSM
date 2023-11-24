#!/bin/bash

#sudo ./vm_setup.sh
#sudo ./mv.sh
#sudo ./setup_total.sh
#sudo ./load_total.sh

#sudo ./run_total.sh $1
#python /home/osm/test_1/python/make_csv.py $1
#python /home/osm/test_1/python/make_LLC_csv.py $1
#python /home/osm/test_1/python/make_LLC_graph.py $1
#python /home/osm/test_1/python/make_LLC_p99.py $1

# : '
for workload_type in "a" "b" "c" "d"
do
    for workload_size in 1000
    do
 	echo "load $workload_type $workload_size"
	sudo ./load_total.sh $workload_type $workload_size
	wait
	for ksm_type in dsa_ksm ksm no_ksm
	do
	    echo "run $ksm_type $workload_type $workload_size"
	    sudo ./run_total.sh $ksm_type $workload_type $workload_size
        done
    done
done
# '

#: '
for workload_type in "a" "b" "c" "d"
do
    for workload_size in 1000
    do
	for ksm_type in no_ksm ksm dsa_ksm
	do
 	    echo "make $ksm_type $workload_type $workload_size"
	    python /home/osm/test_1/python/make_csv.py $ksm_type $workload_type $workload_size 
	    python /home/osm/test_1/python/make_LLC_csv.py $ksm_type $workload_type $workload_size
	    python /home/osm/test_1/python/make_LLC_p99.py $ksm_type $workload_type $workload_size
            python /home/osm/test_1/python/make_LLC_graph.py $ksm_type $workload_type $workload_size &
	done
    done
done
#'
