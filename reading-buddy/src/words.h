// ============================================================
//  WORD LIST  —  edit freely!
//
//  How to mark up a word:
//    -      splits syllables       "but-ter-fly"
//    [ ]    marks tricky letters   "fr[ie]nd", "l[augh]"
//  You can use both:               "e-n[ough]"
//
//  Tip: swap in words from her spelling list or ones she stumbles on.
//  Progress resets automatically when you change this list.
// ============================================================
#pragma once

const char* const WORDS[] = {
  // tricky sight words
  "be-c[au]se", "fr[ie]nd", "s[ai]d", "l[augh]", "th[ough]t",
  "thr[ough]", "e-n[ough]", "b[u]-sy", "a-g[ai]n", "an-y",
  "man-y", "wh[o]", "wh[ere]", "w[ere]", "on[ce]",
  "p[eo]-ple", "s[ure]", "t[ough]", "c[ou]ld", "w[ou]ld",
  "sh[ou]ld", "b[ough]t", "ca[ugh]t", "wr[i]te", "[kn]ow",

  // 2-syllable words
  "an-i-mal", "pic-t[ure]", "sur-prise", "ex-plain", "mid-dle",
  "sud-den", "hap-pen", "an-swer", "hun-gry", "prob-lem",
  "hab-it", "rab-bit", "fin-ish", "win-dow", "mon-ster",
  "gar-den", "fol-low", "hel-met", "nap-kin", "pen-cil",

  // 3+ syllable words
  "beau-ti-ful", "dif-fer-ent", "im-por-tant", "fa-vor-ite", "to-mor-row",
  "ad-ven-t[ure]", "cel-e-brate", "en-er-gy", "won-der-ful", "choc-o-late",
  "sud-den-ly", "va-ca-t[io]n", "an-oth-er", "re-mem-ber", "fam-i-ly",
  "o-[cea]n", "dan-ger-ous", "de-li-[cious]", "ex-cit-ing", "com-put-er",
};

const int NUM_WORDS = sizeof(WORDS) / sizeof(WORDS[0]);
