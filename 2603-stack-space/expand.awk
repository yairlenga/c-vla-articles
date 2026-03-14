#! /bin/awk -f

/^#include.*".*"$/ {
	file=$2
	gsub(/"/, "", file)
	while (( getline line < file ) > 0 ) print line
	close(file)
	next
}
{ print }
