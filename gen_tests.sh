#!/bin/bash

mkdir -p tests 
cd tests

mkdir test_with_different_rights
cd ./test_with_different_rights
for i in {0..7} ; do rm -rf $i; mkdir $i ; for j in {0..7} ; do echo $j > $i/$j; chmod $j$j$j $i/$j ; done ; chmod $i$i$i $i; done
cd ..

mkdir symlink_test
cd ./symlink_test
touch example_file
mkdir -p folder
ln -s ../example_file folder/my_symlink
cd ..

mkdir -p no_dublicates
cd ./no_dublicates
for j in {0..100} ; do echo $j > ./$j; done;
cd ..

mkdir -p same_files
cd ./same_files
for j in {0..10} ; do
    echo "kukarek" > ./same;
    mkdir -p $j;
    cd ./$j
done
for j in {0..11} ; do
    cd ..
done

mkdir -p empty_test

mkdir -p many_groups_of_dublicates
cd ./many_groups_of_dublicates
for i in {0..5} ; do mkdir $i; for j in {0..10} ; do echo $j > $i/$j+$i; done
done

cd ..