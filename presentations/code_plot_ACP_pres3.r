# Data Handling
library(dplyr)
library(forcats)

# Visualisation
library(corrplot)
library(ggplot2)
library(scales)
library(plotly)

# PCA
library(FactoMineR)
library(factoextra)

# Clear variables
rm(list=ls())

#### Informations available from the contests datasets and results ####
data_0001 <- c(
    "bestKnownScore" = 1005593, "contestScore" = 1002232, "GreedyBase" = 646788, "Greedy90" = 646802, 
    "GreedyRR90_10_100" = 787539, "GreedyRRK90_10_100" = 909621, "GreedyRRK90_3_100" = 917349, "GreedyRRK85_5_85" = 985059,
    "V" = 356, "E" = 240, "R" = 396, "R*" = 396, "dupRt" = 0, "C" = 240, "X" = 100, 
    "availCap" = 24000, "capPres" = 1.265, "VpC" = 1.9, "CandVolpC" = 1.1, "V1MoRt" = 0.135, "GiniTotVpR" = 0.54,
    "medEnumpV" = 1, "CpWR" = 1.6, "minVSize" = 1, "maxVSize" = 100, "VsizeRange" = 99
)

data_0002 <- c(
    "bestKnownScore" = 986110, "contestScore" = 982507, "GreedyBase" = 968172, "Greedy90" = 968668, 
    "GreedyRR90_10_100" = 981146, "GreedyRRK90_10_100" = 979748, "GreedyRRK90_3_100" = 976841, "GreedyRRK85_5_85" = 984635,
    "V" = 384, "E" = 192, "R" = 2012, "R*" = 2012, "dupRt" = 0, "C" = 48, "X" = 240, 
    "availCap" = 11520, "capPres" = 0.443, "VpC" = 3.5, "CandVolpC" = 8.6, "V1MoRt" = 0, "GiniTotVpR" = 0.38,
    "medEnumpV" = 6, "CpWR" = 2.0, "minVSize" = 22, "maxVSize" = 115, "VsizeRange" = 93
)

data_0003 <- c(
    "bestKnownScore" = 2550000, "contestScore" = 2550000, "GreedyBase" = 2550000, "Greedy90" = 2550000, 
    "GreedyRR90_10_100" = 2550000, "GreedyRRK90_10_100" = 2550000, "GreedyRRK90_3_100" = 2550000, "GreedyRRK85_5_85" = 2550000,
    "V" = 500, "E" = 50, "R" = 25000, "R*" = 25000, "dupRt" = 0, "C" = 50, "X" = 1000, 
    "availCap" = 50000, "capPres" = 1, "VpC" = 10, "CandVolpC" = 50, "V1MoRt" = 0, "GiniTotVpR" = 0.02,
    "medEnumpV" = 50, "CpWR" = 50, "minVSize" = 100, "maxVSize" = 100, "VsizeRange" = 0
)

data_0004 <- c(
    "bestKnownScore" = 1958747, "contestScore" = 1956202, "GreedyBase" = 1902183, "Greedy90" = 1902959, 
    "GreedyRR90_10_100" = 1947965, "GreedyRRK90_10_100" = 1950799, "GreedyRRK90_3_100" = 1925635, "GreedyRRK85_5_85" = 1956202,
    "V" = 1000, "E" = 100, "R" = 1000, "R*" = 899, "dupRt" = 0.101, "C" = 250, "X" = 1800, 
    "availCap" = 450000, "capPres" = 3.321, "VpC" = 4.5, "CandVolpC" = 1.8, "V1MoRt" = 0, "GiniTotVpR" = 0.64,
    "medEnumpV" = 1, "CpWR" = 2.8, "minVSize" = 2, "maxVSize" = 800, "VsizeRange" = 798
)

data_0005 <- c(
    "bestKnownScore" = 5308417, "contestScore" = 5297522, "GreedyBase" = 5251267, "Greedy90" = 5251267, 
    "GreedyRR90_10_100" = 5292867, "GreedyRRK90_10_100" = 5293641, "GreedyRRK90_3_100" = 5289544, "GreedyRRK85_5_85" = 5297270,
    "V" = 1000, "E" = 100, "R" = 90000, "R*" = 90000, "dupRt" = 0, "C" = 100, "X" = 10000, 
    "availCap" = 1000000, "capPres" = 1.001, "VpC" = 10, "CandVolpC" = 99.9, "V1MoRt" = 0, "GiniTotVpR" = 0.1,
    "medEnumpV" = 100, "CpWR" = 100, "minVSize" = 1000, "maxVSize" = 1000, "VsizeRange" = 0
)

data_0006 <- c(
    "bestKnownScore" = 2355465, "contestScore" = 1542411, "GreedyBase" = 1542411, "Greedy90" = 1542411, 
    "GreedyRR90_10_100" = 1542411, "GreedyRRK90_10_100" = 1542411, "GreedyRRK90_3_100" = 1542411, "GreedyRRK85_5_85" = 1542411,
    "V" = 1471, "E" = 361, "R" = 2424, "R*" = 1741, "dupRt" = 0.282, "C" = 60, "X" = 600000000, 
    "availCap" = 36000000000, "capPres" = 0.952, "VpC" = 15.2, "CandVolpC" = 1.6, "V1MoRt" = 0, "GiniTotVpR" = 0.69,
    "medEnumpV" = 1, "CpWR" = 2.7, "minVSize" = 1000000, "maxVSize" = 966000000, "VsizeRange" = 965000000
)

data_0007 <- c(
    "bestKnownScore" = 1094813, "contestScore" = 1094813, "GreedyBase" = 1094813, "Greedy90" = 1094801, 
    "GreedyRR90_10_100" = 1094813, "GreedyRRK90_10_100" = 1094813, "GreedyRRK90_3_100" = 1094813, "GreedyRRK85_5_85" = 1094813,
    "V" = 1760, "E" = 120, "R" = 7316, "R*" = 7316, "dupRt" = 0, "C" = 31, "X" = 500, 
    "availCap" = 15500, "capPres" = 0.109, "VpC" = 6.2, "CandVolpC" = 78.5, "V1MoRt" = 0, "GiniTotVpR" = 0.73,
    "medEnumpV" = 1, "CpWR" = 4, "minVSize" = 20, "maxVSize" = 481, "VsizeRange" = 461
)

data_0008 <- c(
    "bestKnownScore" = 53633, "contestScore" = 53383, "GreedyBase" = 53125, "Greedy90" = 53156, 
    "GreedyRR90_10_100" = 53411, "GreedyRRK90_10_100" = 53390, "GreedyRRK90_3_100" = 53296, "GreedyRRK85_5_85" = 53370,
    "V" = 3000, "E" = 80, "R" = 216017, "R*" = 216017, "dupRt" = 0, "C" = 15, "X" = 2000, 
    "availCap" = 30000, "capPres" = 0.039, "VpC" = 7.8, "CandVolpC" = 384.1, "V1MoRt" = 0, "GiniTotVpR" = 0.02,
    "medEnumpV" = 72, "CpWR" = 11.8, "minVSize" = 10, "maxVSize" = 500, "VsizeRange" = 490
)

data_0009 <- c(
    "bestKnownScore" = 1100741, "contestScore" = 1022070, "GreedyBase" = 563835, "Greedy90" = 563835, 
    "GreedyRR90_10_100" = 570653, "GreedyRRK90_10_100" = 1022332, "GreedyRRK90_3_100" = 1022332, "GreedyRRK85_5_85" = 1022070,
    "V" = 3000, "E" = 1000, "R" = 3600, "R*" = 3600, "dupRt" = 0, "C" = 1000, "X" = 1998, 
    "availCap" = 1998000, "capPres" = 0.666, "VpC" = 2, "CandVolpC" = 1, "V1MoRt" = 0, "GiniTotVpR" = 0.14,
    "medEnumpV" = 1, "CpWR" = 1.3, "minVSize" = 999, "maxVSize" = 1000, "VsizeRange" = 1
)

data_0010 <- c(
    "bestKnownScore" = 582898, "contestScore" = 581696, "GreedyBase" = 580599, "Greedy90" = 580588, 
    "GreedyRR90_10_100" = 581677, "GreedyRRK90_10_100" = 581628, "GreedyRRK90_3_100" = 581579, "GreedyRRK85_5_85" = 581665,
    "V" = 5000, "E" = 500, "R" = 200289, "R*" = 200289, "dupRt" = 0, "C" = 200, "X" = 385, 
    "availCap" = 77000, "capPres" = 0.229, "VpC" = 5.7, "CandVolpC" = 839.9, "V1MoRt" = 0.023, "GiniTotVpR" = 0.15,
    "medEnumpV" = 40, "CpWR" = 33, "minVSize" = 1, "maxVSize" = 300, "VsizeRange" = 299
)

data_0011 <- c(
    "bestKnownScore" = 133408, "contestScore" = 132042, "GreedyBase" = 131569, "Greedy90" = 131568, 
    "GreedyRR90_10_100" = 131995, "GreedyRRK90_10_100" = 131983, "GreedyRRK90_3_100" = 131960, "GreedyRRK85_5_85" = 132066,
    "V" = 8000, "E" = 110, "R" = 399997, "R*" = 29458, "dupRt" = 0.926, "C" = 40, "X" = 3000, 
    "availCap" = 120000, "capPres" = 0.25, "VpC" = 48.9, "CandVolpC" = 50.6, "V1MoRt" = 0, "GiniTotVpR" = 0.29,
    "medEnumpV" = 4, "CpWR" = 4, "minVSize" = 4, "maxVSize" = 119, "VsizeRange" = 115
)

data_0012 <- c(
    "bestKnownScore" = 331811, "contestScore" = 329758, "GreedyBase" = 328677, "Greedy90" = 328669, 
    "GreedyRR90_10_100" = 329557, "GreedyRRK90_10_100" = 329740, "GreedyRRK90_3_100" = 329746, "GreedyRRK85_5_85" = 329758,
    "V" = 8000, "E" = 2196, "R" = 136928, "R*" = 122705, "dupRt" = 0.104, "C" = 409, "X" = 50000, 
    "availCap" = 20450000, "capPres" = 1.682, "VpC" = 32.9, "CandVolpC" = 10, "V1MoRt" = 0, "GiniTotVpR" = 0.42,
    "medEnumpV" = 12, "CpWR" = 2.3, "minVSize" = 50, "maxVSize" = 3000, "VsizeRange" = 2950
)

data_0013 <- c(
    "bestKnownScore" = 5066500, "contestScore" = 5066462, "GreedyBase" = 5066500, "Greedy90" = 5066222, 
    "GreedyRR90_10_100" = 5066490, "GreedyRRK90_10_100" = 5066482, "GreedyRRK90_3_100" = 5066496, "GreedyRRK85_5_85" = 5066462,
    "V" = 10000, "E" = 100, "R" = 1000000, "R*" = 1000000, "dupRt" = 0, "C" = 0, "X" = 100, 
    "availCap" = 500000, "capPres" = 1, "VpC" = 100, "CandVolpC" = 100, "V1MoRt" = 0, "GiniTotVpR" = 0.01,
    "medEnumpV" = 100, "CpWR" = 100, "minVSize" = 5000, "maxVSize" = 5000, "VsizeRange" = 0
)

data_0014 <- c(
    "bestKnownScore" = 57786, "contestScore" = 57482, "GreedyBase" = 57197, "Greedy90" = 57194, 
    "GreedyRR90_10_100" = 57480, "GreedyRRK90_10_100" = 57462, "GreedyRRK90_3_100" = 57466, "GreedyRRK85_5_85" = 57482,
    "V" = 10000, "E" = 115, "R" = 998964, "R*" = 72538, "dupRt" = 0.927, "C" = 20, "X" = 2000, 
    "availCap" = 40000, "capPres" = 0.071, "VpC" = 35.5, "CandVolpC" = 219.7, "V1MoRt" = 0, "GiniTotVpR" = 0.21,
    "medEnumpV" = 7, "CpWR" = 4, "minVSize" = 4, "maxVSize" = 109, "VsizeRange" = 105
)

data_0015 <- c(
    "bestKnownScore" = 292212, "contestScore" = 292212, "GreedyBase" = 222004, "Greedy90" = 222004, 
    "GreedyRR90_10_100" = 222052, "GreedyRRK90_10_100" = 292212, "GreedyRRK90_3_100" = 292212, "GreedyRRK85_5_85" = 292212,
    "V" = 10000, "E" = 1000, "R" = 714629, "R*" = 714629, "dupRt" = 0, "C" = 100, "X" = 1000, 
    "availCap" = 100000, "capPres" = 0.022, "VpC" = 2.2, "CandVolpC" = 4371.3, "V1MoRt" = 0.007, "GiniTotVpR" = 0.35,
    "medEnumpV" = 86, "CpWR" = 3.8, "minVSize" = 1, "maxVSize" = 1000, "VsizeRange" = 999
)

#### INSTANCES FROM BEFORE ####
data_9901 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 100, "E" = 10, "R" = 100, "R*" = 81, "dupRt" = 0.19, "C" = 10, "X" = 100, "availCap" = 1000,
    "capPres" = 1.127, "VpC"= 4.0, "V1MoRt" = 0.03, "GiniTotVpR" = 0.57, "medEnumpV" = 1,
    "CpWR" = 3.4, "contestScoreRatio" = 0.9833, "CandVolpC" = 4.7, "minVSize" = 1, "maxVSize" = 50, "VsizeRange" = 49
)

data_9902 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 10000, "E" = 100, "R" = 100000, "R*" = 40317, "dupRt" = 0.597, "C" = 100, "X" = 10000, "availCap" = 1000000,
    "capPres" = 0.465, "VpC"= 33.3, "V1MoRt" = 0.002, "GiniTotVpR" = 0.86, "medEnumpV" = 2,
    "CpWR" = 5.2, "contestScoreRatio" = 0.9952, "CandVolpC" = 40.8, "minVSize" = 1, "maxVSize" = 600, "VsizeRange" = 599
)

data_9903 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 10000, "E" = 100, "R" = 100000, "R*" = 95180, "dupRt" = 0.048, "C" = 100, "X" = 50000, "availCap" = 5000000,
    "capPres" = 1.000, "VpC"= 100.0, "V1MoRt" = 0, "GiniTotVpR" = 0.21, "medEnumpV" = 9,
    "CpWR" = 100, "contestScoreRatio" = 0.9999, "CandVolpC" = 100, "minVSize" = 88, "maxVSize" = 1000, "VsizeRange" = 912
)

data_9904 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 10000, "E" = 1000, "R" = 200000, "R*" = 197978, "dupRt" = 0.01, "C" = 500, "X" = 6000, "availCap" = 3000000,
    "capPres" = 0.600, "VpC"= 12.0, "V1MoRt" = 0.001, "GiniTotVpR" = 0.14, "medEnumpV" = 20,
    "CpWR" = 351.9, "contestScoreRatio" = 0.9975, "CandVolpC" = 833.1, "minVSize" = 1, "maxVSize" = 1000, "VsizeRange" = 999
)

data_9905 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 10000, "E" = 115, "R" = 998968, "R*" = 72450, "dupRt" = 0.927, "C" = 20, "X" = 2000, "availCap" = 40000,
    "capPres" = 0.071, "VpC"= 35.6, "V1MoRt" = 0, "GiniTotVpR" = 0.21, "medEnumpV" = 7,
    "CpWR" = 4, "contestScoreRatio" = 1, "CandVolpC" = 212.7, "minVSize" = 4, "maxVSize" = 109, "VsizeRange" = 105
)

data_9906 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 10000, "E" = 115, "R" = 717907, "R*" = 717907, "dupRt" = 0, "C" = 20, "X" = 2000, "availCap" = 40000,
    "capPres" = 0.021, "VpC"= 10.4, "V1MoRt" = 0.669, "GiniTotVpR" = 0.50, "medEnumpV" = 74,
    "CpWR" = 12.7, "contestScoreRatio" = 1, "CandVolpC" = 960.1, "minVSize" = 1, "maxVSize" = 1000, "VsizeRange" = 999
)

data_9907 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 10000, "E" = 500, "R" = 100000, "R*" = 98992, "dupRt" = 0.01, "C" = 100, "X" = 250000, "availCap" = 25000000,
    "capPres" = 4.977, "VpC"= 497.7, "V1MoRt" = 0, "GiniTotVpR" = 0.18, "medEnumpV" = 10,
    "CpWR" = 85, "contestScoreRatio" = 1, "CandVolpC" = 20.1, "minVSize" = 1, "maxVSize" = 1000, "VsizeRange" = 999
)

data_9908 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 2, "E" = 2, "R" = 2, "R*" = 2, "dupRt" = 0, "C" = 1, "X" = 20, "availCap" = 20,
    "capPres" = 0.952, "VpC"= 1.9, "V1MoRt" = 0.5, "GiniTotVpR" = 0.45, "medEnumpV" = 1,
    "CpWR" = 1, "contestScoreRatio" = 0.0501, "CandVolpC" = 1.1, "minVSize" = 1, "maxVSize" = 20, "VsizeRange" = 19
)

data_9909 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 20, "E" = 2, "R" = 20, "R*" = 20, "dupRt" = 0, "C" = 2, "X" = 200, "availCap" = 400,
    "capPres" = 1.050, "VpC"= 10.5, "V1MoRt" = 0.05, "GiniTotVpR" = 0.05, "medEnumpV" = 1,
    "CpWR" = 1.5, "contestScoreRatio" = 0.9052, "CandVolpC" = 1.4, "minVSize" = 1, "maxVSize" = 20, "VsizeRange" = 19
)

data_9910 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 3860, "E" = 246, "R" = 84901, "R*" = 81146, "dupRt" = 0.044, "C" = 66, "X" = 250000, "availCap" = 16500000,
    "capPres" = 8.602, "VpC" = 503.1, "V1MoRt" = 0.001, "GiniTotVpR" = 0.12, "medEnumpV" = 21,
    "CpWR" = 56, "contestScoreRatio" = 0.9994, "CandVolpC" = 7.7, "minVSize" = 1, "maxVSize" = 1000, "VsizeRange" = 999
)

data_9911 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 4, "E" = 2, "R" = 4, "R*" = 4, "dupRt" = 0, "C" = 2, "X" = 40, "availCap" = 80,
    "capPres" = 1.311, "VpC"= 2.6, "V1MoRt" = 0.25, "GiniTotVpR" = 0.23, "medEnumpV" = 1,
    "CpWR" = 1.3, "contestScoreRatio" = 0.5130, "CandVolpC" = 1, "minVSize" = 1, "maxVSize" = 20, "VsizeRange" = 19
)

data_9912 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 4990, "E" = 246, "R" = 84901, "R*" = 78530, "dupRt" = 0.075, "C" = 50, "X" = 85000, "availCap" = 4250000,
    "capPres" = 1.707, "VpC"= 170.3, "V1MoRt" = 0.001, "GiniTotVpR" = 0.38, "medEnumpV" = 12,
    "CpWR" = 20, "contestScoreRatio" = 0.9999, "CandVolpC" = 28.9, "minVSize" = 1, "maxVSize" = 1000, "VsizeRange" = 999
)

data_9913 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 6, "E" = 2, "R" = 6, "R*" = 6, "dupRt" = 0, "C" = 2, "X" = 3, "availCap" = 6,
    "capPres" = 1, "VpC"= 3, "V1MoRt" = 1, "GiniTotVpR" = 0, "medEnumpV" = 1,
    "CpWR" = 1.5, "contestScoreRatio" = 0.6003, "CandVolpC" = 1.5, "minVSize" = 1, "maxVSize" = 1, "VsizeRange" = 0
)

data_9914 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 6970, "E" = 311, "R" = 100000, "R*" = 94583, "dupRt" = 0.054, "C" = 69, "X" = 200000, "availCap" = 13800000,
    "capPres" = 3.949, "VpC"= 398.9, "V1MoRt" = 0.001, "GiniTotVpR" = 0.38, "medEnumpV" = 11,
    "CpWR" = 10, "contestScoreRatio" = 0.9999, "CandVolpC" = 13.8, "minVSize" = 1, "maxVSize" = 1000, "VsizeRange" = 999
)

data_9915 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 8, "E" = 2, "R" = 8, "R*" = 8, "dupRt" = 0, "C" = 2, "X" = 80, "availCap" = 160,
    "capPres" = 1.135, "VpC" = 4.5, "V1MoRt" = 0.125, "GiniTotVpR" = 0.12, "medEnumpV" = 1,
    "CpWR" = 1.4, "contestScoreRatio" = 0.5050, "CandVolpC" = 1.3, "minVSize" = 1, "maxVSize" = 20, "VsizeRange" = 19
)

data_9916 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 6891, "E" = 865, "R" = 8762, "R*" = 8762, "dupRt" = 0, "C" = 500, "X" = 1000, "availCap" = 500000,
    "capPres" = 0.564, "VpC"= 7.8, "V1MoRt" = 0, "GiniTotVpR" = 0.49, "medEnumpV" = 1,
    "CpWR" = 1, "contestScoreRatio" = 0.9208, "CandVolpC" = 1.7, "minVSize" = 17, "maxVSize" = 1000, "VsizeRange" = 983
)

data_9917 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 10000, "E" = 500, "R" = 100000, "R*" = 100000, "dupRt" = 0, "C" = 100, "X" = 1000, "availCap" = 100000,
    "capPres" = 0.022, "VpC"= 2.2, "V1MoRt" = 0.1, "GiniTotVpR" = 0.89, "medEnumpV" = 10,
    "CpWR" = 10, "contestScoreRatio" = 0.9203, "CandVolpC" = 2797, "minVSize" = 1, "maxVSize" = 1000, "VsizeRange" = 999
)

data_9918 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 10000, "E" = 500, "R" = 100000, "R*" = 100000, "dupRt" = 0, "C" = 100, "X" = 100000, "availCap" = 10000000,
    "capPres" = 0.024, "VpC"= 2.4, "V1MoRt" = 0.016, "GiniTotVpR" = 0.81, "medEnumpV" = 10,
    "CpWR" = 10, "contestScoreRatio" = 0.1168, "CandVolpC" = 2575.8, "minVSize" = 1, "maxVSize" = 1000, "VsizeRange" = 999
)

data_9919 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 8000, "E" = 110, "R" = 399997, "R*" = 29458, "dupRt" = 0.926, "C" = 40, "X" = 3000, "availCap" = 120000,
    "capPres" = 0.250, "VpC"= 48.9, "V1MoRt" = 0, "GiniTotVpR" = 0.29, "medEnumpV" = 4,
    "CpWR" = 4, "contestScoreRatio" = 0.5267, "CandVolpC" = 50.6, "minVSize" = 4, "maxVSize" = 119, "VsizeRange" = 105
)

data_9920 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 600, "E" = 100, "R" = 37408, "R*" = 37408, "dupRt" = 0, "C" = 40, "X" = 8000, "availCap" = 320000,
    "capPres" = 0.630, "VpC"= 9.5, "V1MoRt" = 0, "GiniTotVpR" = 0.5, "medEnumpV" = 63,
    "CpWR" = 24.7, "contestScoreRatio" = 0.9914, "CandVolpC" = 63.5, "minVSize" = 1, "maxVSize" = 446, "VsizeRange" = 1000
)

data_9921 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 3000, "E" = 80, "R" = 640075, "R*" = 233021, "dupRt" = 0.652, "C" = 15, "X" = 2000, "availCap" = 30000,
    "capPres" = 0.177, "VpC"= 35.3, "V1MoRt" = 0, "GiniTotVpR" = 0.04, "medEnumpV" = 75,
    "CpWR" = 8, "contestScoreRatio" = 1, "CandVolpC" = 85, "minVSize" = 4, "maxVSize" = 109, "VsizeRange" = 105
)

data_9922 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 3000, "E" = 80, "R" = 215637, "R*" = 215637, "dupRt" = 0, "C" = 15, "X" = 2000, "availCap" = 30000,
    "capPres" = 0.039, "VpC"= 7.8, "V1MoRt" = 0, "GiniTotVpR" = 0.02, "medEnumpV" = 72,
    "CpWR" = 3.2, "contestScoreRatio" = 1, "CandVolpC" = 385.3, "minVSize" = 10, "maxVSize" = 500, "VsizeRange" = 490
)

data_9923 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 3000, "E" = 80, "R" = 106790, "R*" = 106790, "dupRt" = 0, "C" = 15, "X" = 2000, "availCap" = 30000,
    "capPres" = 0.039, "VpC"= 7.8, "V1MoRt" = 0, "GiniTotVpR" = 0.38, "medEnumpV" = 36,
    "CpWR" = 7.2, "contestScoreRatio" = 1, "CandVolpC" = 384, "minVSize" = 10, "maxVSize" = 500, "VsizeRange" = 490
)

data_9924 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 3000, "E" = 80, "R" = 640030, "R*" = 165599, "dupRt" = 0.741, "C" = 15, "X" = 2000, "availCap" = 30000,
    "capPres" = 0.180, "VpC"= 35.2, "V1MoRt" = 0, "GiniTotVpR" = 0.5, "medEnumpV" = 70,
    "CpWR" = 8, "contestScoreRatio" = 1, "CandVolpC" = 81.9, "minVSize" = 4, "maxVSize" = 109, "VsizeRange" = 105
)

data_9925 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 3000, "E" = 80, "R" = 54235, "R*" = 54235, "dupRt" = 0, "C" = 15, "X" = 2000, "availCap" = 30000,
    "capPres" = 0.039, "VpC"= 7.8, "V1MoRt" = 0, "GiniTotVpR" = 0.61, "medEnumpV" = 16,
    "CpWR" = 3, "contestScoreRatio" = 1, "CandVolpC" = 368.8, "minVSize" = 10, "maxVSize" = 500, "VsizeRange" = 490
)

data_9926 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 10000, "E" = 1000, "R" = 200000, "R*" = 200000, "dupRt" = 0, "C" = 500, "X" = 10000, "availCap" = 5000000,
    "capPres" = 2.481, "VpC"= 48.2, "V1MoRt" = 0, "GiniTotVpR" = 0.77, "medEnumpV" = 5,
    "CpWR" = 59.2, "contestScoreRatio" = 0.9974, "CandVolpC" = 66.1, "minVSize" = 10, "maxVSize" = 950, "VsizeRange" = 940
)

data_9927 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 10000, "E" = 1000, "R" = 200000, "R*" = 200000, "dupRt" = 0, "C" = 500, "X" = 10000, "availCap" = 5000000,
    "capPres" = 2.448, "VpC"= 47.7, "V1MoRt" = 0, "GiniTotVpR" = 0.77, "medEnumpV" = 5,
    "CpWR" = 74.8, "contestScoreRatio" = 0.9997, "CandVolpC" = 121.2, "minVSize" = 10, "maxVSize" = 950, "VsizeRange" = 940
)

data_9928 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 1000, "E" = 100, "R" = 1000, "R*" = 907, "dupRt" = 0.093, "C" = 250, "X" = 1800, "availCap" = 450000,
    "capPres" = 2.744, "VpC"= 4.5, "V1MoRt" = 0, "GiniTotVpR" = 0.54, "medEnumpV" = 1,
    "CpWR" = 3.2, "contestScoreRatio" = 0.9965, "CandVolpC" = 2, "minVSize" = 2, "maxVSize" = 800, "VsizeRange" = 798
)

data_9929 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 1369, "E" = 379, "R" = 1984, "R*" = 1984, "dupRt" = 0, "C" = 200, "X" = 500, "availCap" = 100000,
    "capPres" = 0.578, "VpC"= 4, "V1MoRt" = 0, "GiniTotVpR" = 0.43, "medEnumpV" = 1,
    "CpWR" = 1, "contestScoreRatio" = 0.8977, "CandVolpC" = 1.7, "minVSize" = 21, "maxVSize" = 500, "VsizeRange" = 479
)

data_9930 <- c(
    "bestKnownScore" = NA, "contestScore" = NA, "GreedyBase" = NA, "Greedy90" = NA, 
    "GreedyRR90_10_100" = NA, "GreedyRRK90_10_100" = NA, "GreedyRRK90_3_100" = NA, "GreedyRRK85_5_85" = NA,
    "V" = 1000, "E" = 100, "R" = 1000, "R*" = 899, "dupRt" = 0.101, "C" = 250, "X" = 1800, "availCap" = 450000,
    "capPres" = 3.321, "VpC"= 4.5, "V1MoRt" = 0, "GiniTotVpR" = 0.64, "medEnumpV" = 1,
    "CpWR" = 2.8, "contestScoreRatio" = 0.9918, "CandVolpC" = 1.8, "minVSize" = 2, "maxVSize" = 800, "VsizeRange" = 798
)

##### Form the datasets ####
dataset_names <- ls()

df <- dplyr::bind_rows(list(
    data_0001,
    data_0002,
    data_0003,
    data_0004,
    data_0005,
    data_0006,
    data_0007,
    data_0008,
    data_0009,
    data_0010,
    data_0011,
    data_0012,
    data_0013,
    data_0014,
    data_0015,
    data_9901,
    data_9902,
    data_9903,
    data_9904,
    data_9905,
    data_9906,
    data_9907,
    data_9908,
    data_9909,
    data_9910,
    data_9911,
    data_9912,
    data_9913,
    data_9914,
    data_9915,
    data_9916,
    data_9917,
    data_9918,
    data_9919,
    data_9920,
    data_9921,
    data_9922,
    data_9923,
    data_9924,
    data_9925,
    data_9926,
    data_9927,
    data_9928,
    data_9929,
    data_9930
))

# Add ratio to best score
emptyScoreRatio <- which(is.na(df$contestScoreRatio))
df$contestScoreRatio[emptyScoreRatio] <- df$contestScore[emptyScoreRatio]/df$bestKnownScore[emptyScoreRatio]

# Remove cols of df for analysis
df <- df %>%
    select(-c(`CandVolpC`, `CpWR`))

# Add rownames
suppressWarnings(rownames(df) <- dataset_names)

#### Analysis ####
##### Evolution on competition ######
# Form  the dataset of competition instances
dfHist <- df
dfHist$instance <- rownames(dfHist)
dfHistRaw <- dfHist[which(startsWith(dfHist$instance, "data_00")),]
dfHistRaw <- dfHistRaw %>%
    select(GreedyBase, Greedy90, GreedyRR90_10_100, GreedyRRK85_5_85, GreedyRRK90_10_100, GreedyRRK90_3_100, contestScore) / 
    dfHistRaw$bestKnownScore

ggplot(stack(dfHistRaw) %>% dplyr::group_by(ind) %>% dplyr::summarise(values=mean(values))) +
    geom_col(aes(y=values, x=ind)) +
    labs(
        title = "Ratio moyen au meilleur score",
        subtitle = "VS Instances de la compétition"
    ) +
    xlab("Algorithmes") +
    ylab("Moyenne du ratio au meilleur score") +
    coord_cartesian(ylim=c(0.8, 1)) +
    geom_hline(yintercept = 1, linewidth = 1.1, colour = "orange", linetype = "dashed")

dfHistRawInstance <- dfHistRaw
dfHistRawInstance$instance <- as.factor(dfHist[which(startsWith(dfHist$instance, "data_00")),]$instance)

# Évolution du score par instance
ggplot(tidyr::pivot_longer(dfHistRawInstance, !instance, names_to = "Algo")) +
    geom_tile(aes(x=Algo, y=instance, fill=value)) +
    geom_text(aes(x=Algo, y=instance, label=round(value, 6))) +
    scale_fill_gradientn(colors=c("salmon","yellow","cadetblue")) +
    xlab("Algorithmes") +
    ylab("Instances de la compétition") +
    labs(
        fill = "Best Score Ratio",
        title = "Ratio au meilleur score par algorithme",
        subtitle = "VS Instances de la compétition"
        ) +
    theme(
        legend.background = element_rect(fill = "gray", color = "black"),
        legend.box.background = element_rect(fill = "gray", linewidth = 1),
        legend.title = element_text(angle = 90),
        legend.title.position = "left"
    )


##### Final Ranking ######

##### PCA Without competition #####
# Initialise dataset for the the pca
dfPCA <- df
dfPCA$instance <- rownames(dfPCA)

# Do the PCA with competitive instances as quali.sup
is.ind.sup.mask <- startsWith(dfPCA$instance, "data_00")
is.ind.sup <- which(is.ind.sup.mask)
res.pca <- suppressWarnings(FactoMineR::PCA(
    dfPCA,
    scale.unit = T,
    ind.sup = is.ind.sup,
    quali.sup = "instance",
    quanti.sup = c("bestKnownScore", "contestScore", "contestScoreRatio", "GreedyBase", "Greedy90", "GreedyRR90_10_100", "GreedyRRK90_10_100", "GreedyRRK90_3_100", "GreedyRRK85_5_85", "maxVSize", "minVSize", "VsizeRange"),
    graph = F
))

# Plot without competition instances for the variable
factoextra::fviz_pca_var(res.pca, axes = c(1,2), invisible = "quanti.sup")
factoextra::fviz_contrib(res.pca, "var", axes = 1)
factoextra::fviz_cos2(res.pca, "var", axes = 1)
factoextra::fviz_contrib(res.pca, "var", axes = 2)
factoextra::fviz_cos2(res.pca, "var", axes = 2)
factoextra::fviz_contrib(res.pca, "var", axes = 3)
factoextra::fviz_cos2(res.pca, "var", axes = 3)
factoextra::fviz_contrib(res.pca, "var", axes = 4)
factoextra::fviz_cos2(res.pca, "var", axes = 4)
factoextra::fviz_contrib(res.pca, "var", axes = 5)
factoextra::fviz_cos2(res.pca, "var", axes = 5)


# Get ind sup based on coordinate
getIndSupWithoutExtreme <- function(val) {
    return(as.data.frame(cbind(res.pca$ind.sup$coord, dfPCA$contestScoreRatio[is.ind.sup], "instance"=is.ind.sup)) %>% filter(abs(Dim.1)<val, abs(Dim.2)<val))
}

# Plot Ind pca based on possible coordinates
plotIndPCA <- function(res.pca, val, axes = c(1,2)) {
    ind.sup <- getIndSupWithoutExtreme(val)
    factoextra::fviz_pca_ind(res.pca, axes=axes, invisible = "ind.sup", geom = "point", pointshape = 15, pointsize = 2.5, col.ind = dfPCA$contestScoreRatio[!is.ind.sup.mask]) +
        scale_color_gradient(low="red", high = "blue") +
        labs(
            title = "Analyse en Composante Principale des Métriques",
            subtitle = "Graphe des Individus VS Best Score Ratio",
            colour = "Best Score Ratio"
        ) +
        theme(
            legend.box.background = element_rect(fill = "gray", linewidth = 1),
            legend.title = element_text(angle = 90),
            legend.title.position = "left"
        ) +
        geom_point(data = ind.sup, aes(x=Dim.1, y=Dim.2, colour = V6, size = I(2.5), shape = I(17))) +
        geom_text(data = ind.sup, aes(x=Dim.1, y=Dim.2, label = instance), vjust = -0.75)
}

# Without instance 13 and 6
plotIndPCA(res.pca, 10)

# Without instance 6
plotIndPCA(res.pca, 300)

# With all instances
plotIndPCA(res.pca, 10E6)


