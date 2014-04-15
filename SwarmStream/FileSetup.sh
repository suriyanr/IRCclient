#set -x
rm -f Swarm.dat

MAXID=40

if [ "$1" == "clean" ]; then
  for (( i=0; i<= $MAXID; i++ ));
  do
    rm -rf $i
  done
  exit
fi

for (( i=0; i<= $MAXID; i++ ));
do
   echo "Creating Directory: $i"
   mkdir -p $i
   echo "Creating File $i/Swarm.avi"
   RANDOM=$i
   cut_filesize=$((RANDOM*6))
   rm -f $i/Swarm.avi
   head -c $cut_filesize Swarm.avi > $i/Swarm.avi
   echo "Nick$i $i Swarm.avi 8000 20000 N" >> Swarm.dat
done
