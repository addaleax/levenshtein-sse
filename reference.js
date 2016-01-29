#!/usr/bin/env node
'use strict';

// from https://github.com/hiddentao/fast-levenshtein
// (MIT, Copyright (c) 2013 Ramesh Nair)
function levenshtein(str1, str2) {
  // base cases
  if (str1 === str2) return 0;
  if (str1.length === 0) return str2.length;
  if (str2.length === 0) return str1.length;

  // two rows
  let prevRow  = new Array(str2.length + 1),
      curCol, nextCol, i, j, tmp;

  // initialise previous row
  for (i=0; i<prevRow.length; ++i) {
    prevRow[i] = i;
  }
  
  console.log(0, prevRow.join(' '));
  // calculate current row distance from previous row
  for (i=0; i<str1.length; ++i) {
    nextCol = i + 1;

    for (j=0; j<str2.length; ++j) {
      curCol = nextCol;

      // substution
      nextCol = prevRow[j] + ( (str1[i] === str2[j]) ? 0 : 1 );
      // insertion
      tmp = curCol + 1;
      if (nextCol > tmp) {
        nextCol = tmp;
      }
      // deletion
      tmp = prevRow[j + 1] + 1;
      if (nextCol > tmp) {
        nextCol = tmp;
      }

      // copy current col value into previous (in preparation for next iteration)
      prevRow[j] = curCol;
    }

    // copy last col value into previous (in preparation for next iteration)
    prevRow[j] = nextCol;
    
    console.log(i, prevRow.join(' '));
  }

  return nextCol;
};

module.exports = levenshtein;

if (require.main === module) {
  const args = process.argv.slice(2,4);
  console.log('Args:', args);
  const v = levenshtein(args[0], args[1]);
  console.log('Result:', v);
}
