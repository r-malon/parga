#!/bin/sh

# POSIX-compliant shell script to find a collision-free Pearson hash table.
# Reads keywords from stdin, one per line.
# Requires awk, expr, cut, tr (all POSIX standard).

# Read keywords from stdin into space-separated string
keywords=""
while read -r line; do
	if [ -n "$line" ]; then
		keywords="$keywords $line"
	fi
done

max_attempts=1000

i=1
while [ "$i" -le "$max_attempts" ]; do
	input=""
	n=0
	while [ "$n" -le 255 ]; do
		input="$input$n\n"
		n=$((n + 1))
	done

	# Shuffle with awk Fisher-Yates
	table=$(printf "%b" "$input" | awk 'BEGIN { srand() } { lines[NR]=$0 } END { for(k=NR;k>0;k--) { j=int(rand()*k+1); t=lines[j]; lines[j]=lines[k]; lines[k]=t; print lines[k] } }')

	table_spaced=$(echo "$table" | tr '\n' ' ')

	unique_hashes=""
	collision=0

	for kw in $keywords; do
		h=0

		pos=1
		while [ "$pos" -lt "${#kw}" ]; do
			# Get char at pos
			char=$(expr substr "$kw" "$pos" 1)

			char_code=$(printf '%d' "'$char")

			xor=$((h ^ char_code))

			# table[xor] (0-based, but cut -f starts at 1)
			table_val=$(echo "$table_spaced" | cut -d' ' -f$((xor + 1)))

			h="$table_val"

			pos=$((pos + 1))
		done

		case " $unique_hashes " in
			*" $h "*) collision=1; break ;;
		esac
		unique_hashes="$unique_hashes $h"
	done

	if [ "$collision" = 0 ]; then
		echo "/* Collision-free table took $i attempts. */"
		echo "static const unsigned char table[] = {"

		pos=1
		while [ "$pos" -le 256 ]; do
			row=""
			end=$((pos + 16 - 1))
			if [ "$end" -gt 256 ]; then
				end=256
			fi

			f="$pos"
			while [ "$f" -le "$end" ]; do
				val=$(echo "$table_spaced" | cut -d' ' -f"$f")
				row="$row$val, "
				f=$((f + 1))
			done

			echo "	$row"
			pos=$((pos + 16))
		done

		echo "};"
		exit 0
	fi

	i=$((i + 1))
done

echo "Failed to find a collision-free table after $max_attempts attempts."
