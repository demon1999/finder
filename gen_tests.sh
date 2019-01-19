#!/bin/bash

mkdir -p tests 
cd tests

mkdir test_with_different_rights
cd ./test_with_different_rights
for i in {0..7} ; do rm -rf $i; mkdir $i ; for j in {0..7} ; do echo $j$j$j > $i/$j$j$j; chmod $j$j$j $i/$j$j$j ; done ; chmod $i$i$i $i; done
cd ..

mkdir symlink_test
cd ./symlink_test
touch example
echo "example" > ./example
mkdir -p folder
ln -s ../example folder/my_symlink
cd ..

mkdir -p same_files
cd ./same_files
for j in {0..10} ; do
    echo "same" > ./same;
    mkdir -p $j;
    cd ./$j
done
for j in {0..11} ; do
    cd ..
done

mkdir -p empty_test

mkdir -p many_groups_of_same_numbers
cd ./many_groups_of_same_numbers
for i in {0..5} ; do mkdir $i; for j in {0..10} ; do echo $j$j$j > $i/$j$j$j; done
done
cd ..
mkdir -p test_with_interesting_string
echo "abacabadaba" > ./test_with_interesting_string/abacabadaba

cd ..