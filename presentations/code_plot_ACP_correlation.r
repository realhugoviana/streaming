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

#### Informations obtained from the datasets and resolutions ####
# Score ratio as of 09.10.2026 at 21h46 against simple greedy
data_meAtTheZoo <- c(
    "GreedyTime" = 0, "#V" = 100, "#E" = 10, "#R" = 100, "#unique R" = 81, "Dulication Rate" = 0.19, "#C" = 10, "X" = 100, "Available Capacity" = 1000,
    "Capacity Pressure" = 1.127, "Videos Per Cache"= 4.0, "V1Mo Rate" = 0.03, "Gini Total Video Per Request" = 0.57, "Median Endpoint Number Per Video" = 1,
    "Caches Per Weighted Request" = 3.4, "bestScoreRatio" = 0.9833, "Candidate Volume Per Cache" = 4.7
)
data_videosWorthSpreading<- c(
    "GreedyTime" = 29.9, "#V" = 10000, "#E" = 100, "#R" = 100000, "#unique R" = 40317, "Dulication Rate" = 0.597, "#C" = 100, "X" = 10000, "Available Capacity" = 1000000,
    "Capacity Pressure" = 0.465, "Videos Per Cache"= 33.3, "V1Mo Rate" = 0.002, "Gini Total Video Per Request" = 0.86, "Median Endpoint Number Per Video" = 2,
    "Caches Per Weighted Request" = 5.2, "bestScoreRatio" = 0.9952, "Candidate Volume Per Cache" = 40.8
)
data_trendingToday <- c(
    "GreedyTime" = 28.8, "#V" = 10000, "#E" = 100, "#R" = 100000, "#unique R" = 95180, "Dulication Rate" = 0.048, "#C" = 100, "X" = 50000, "Available Capacity" = 5000000,
    "Capacity Pressure" = 1.000, "Videos Per Cache"= 100.0, "V1Mo Rate" = 0, "Gini Total Video Per Request" = 0.21, "Median Endpoint Number Per Video" = 9,
    "Caches Per Weighted Request" = 100, "bestScoreRatio" = 0.9999, "Candidate Volume Per Cache" = 100
)
data_kittens <- c(
    "GreedyTime" = 257.4, "#V" = 10000, "#E" = 1000, "#R" = 200000, "#unique R" = 197978, "Dulication Rate" = 0.01, "#C" = 500, "X" = 6000, "Available Capacity" = 3000000,
    "Capacity Pressure" = 0.600, "Videos Per Cache"= 12.0, "V1Mo Rate" = 0.001, "Gini Total Video Per Request" = 0.14, "Median Endpoint Number Per Video" = 20,
    "Caches Per Weighted Request" = 351.9, "bestScoreRatio" = 0.9975, "Candidate Volume Per Cache" = 833.1
)

##### Custom Instances (from us) ####
data_dejaVu42 <- c(
    "GreedyTime" = 5, "#V" = 10000, "#E" = 115, "#R" = 998968, "#unique R" = 72450, "Dulication Rate" = 0.927, "#C" = 20, "X" = 2000, "Available Capacity" = 40000,
    "Capacity Pressure" = 0.071, "Videos Per Cache"= 35.6, "V1Mo Rate" = 0, "Gini Total Video Per Request" = 0.21, "Median Endpoint Number Per Video" = 7,
    "Caches Per Weighted Request" = 4, "bestScoreRatio" = 1, "Candidate Volume Per Cache" = 212.7
)
data_lambda42 <- c(
    "GreedyTime" = 21.9, "#V" = 10000, "#E" = 115, "#R" = 717907, "#unique R" = 717907, "Dulication Rate" = 0, "#C" = 20, "X" = 2000, "Available Capacity" = 40000,
    "Capacity Pressure" = 0.021, "Videos Per Cache"= 10.4, "V1Mo Rate" = 0.669, "Gini Total Video Per Request" = 0.50, "Median Endpoint Number Per Video" = 74,
    "Caches Per Weighted Request" = 12.7, "bestScoreRatio" = 1, "Candidate Volume Per Cache" = 960.1
)

##### Custom Instances (from others) ####
data_10000_500_100000_100 <- c(
    "GreedyTime" = 592.6, "#V" = 10000, "#E" = 500, "#R" = 100000, "#unique R" = 98992, "Dulication Rate" = 0.01, "#C" = 100, "X" = 250000, "Available Capacity" = 25000000,
    "Capacity Pressure" = 4.977, "Videos Per Cache"= 497.7, "V1Mo Rate" = 0, "Gini Total Video Per Request" = 0.18, "Median Endpoint Number Per Video" = 10,
    "Caches Per Weighted Request" = 85, "bestScoreRatio" = 1, "Candidate Volume Per Cache" = 20.1
)

data_2_2_2_1 <- c(
    "GreedyTime" = 0, "#V" = 2, "#E" = 2, "#R" = 2, "#unique R" = 2, "Dulication Rate" = 0, "#C" = 1, "X" = 20, "Available Capacity" = 20,
    "Capacity Pressure" = 0.952, "Videos Per Cache"= 1.9, "V1Mo Rate" = 0.5, "Gini Total Video Per Request" = 0.45, "Median Endpoint Number Per Video" = 1,
    "Caches Per Weighted Request" = 1, "bestScoreRatio" = 0.0501, "Candidate Volume Per Cache" = 1.1
)

data_20_2_20_2 <- c(
    "GreedyTime" = 0, "#V" = 20, "#E" = 2, "#R" = 20, "#unique R" = 20, "Dulication Rate" = 0, "#C" = 2, "X" = 200, "Available Capacity" = 400,
    "Capacity Pressure" = 1.050, "Videos Per Cache"= 10.5, "V1Mo Rate" = 0.05, "Gini Total Video Per Request" = 0.05, "Median Endpoint Number Per Video" = 1,
    "Caches Per Weighted Request" = 1.5, "bestScoreRatio" = 0.9052, "Candidate Volume Per Cache" = 1.4
)

data_3860_246_84901_66 <- c(
    "GreedyTime" = 130.1, "#V" = 3860, "#E" = 246, "#R" = 84901, "#unique R" = 81146, "Dulication Rate" = 0.044, "#C" = 66, "X" = 250000, "Available Capacity" = 16500000,
    "Capacity Pressure" = 8.602, "Videos Per Cache" = 503.1, "V1Mo Rate" = 0.001, "Gini Total Video Per Request" = 0.12, "Median Endpoint Number Per Video" = 21,
    "Caches Per Weighted Request" = 56, "bestScoreRatio" = 0.9994, "Candidate Volume Per Cache" = 7.7
)

data_4_2_4_2 <- c(
    "GreedyTime" = 0, "#V" = 4, "#E" = 2, "#R" = 4, "#unique R" = 4, "Dulication Rate" = 0, "#C" = 2, "X" = 40, "Available Capacity" = 80,
    "Capacity Pressure" = 1.311, "Videos Per Cache"= 2.6, "V1Mo Rate" = 0.25, "Gini Total Video Per Request" = 0.23, "Median Endpoint Number Per Video" = 1,
    "Caches Per Weighted Request" = 1.3, "bestScoreRatio" = 0.5130, "Candidate Volume Per Cache" = 1
)

data_4990_246_84901_50 <- c(
    "GreedyTime" = 31.9, "#V" = 4990, "#E" = 246, "#R" = 84901, "#unique R" = 78530, "Dulication Rate" = 0.075, "#C" = 50, "X" = 85000, "Available Capacity" = 4250000,
    "Capacity Pressure" = 1.707, "Videos Per Cache"= 170.3, "V1Mo Rate" = 0.001, "Gini Total Video Per Request" = 0.38, "Median Endpoint Number Per Video" = 12,
    "Caches Per Weighted Request" = 20, "bestScoreRatio" = 0.9999, "Candidate Volume Per Cache" = 28.9
)

data_6_2_6_2 <- c(
    "GreedyTime" = 0, "#V" = 6, "#E" = 2, "#R" = 6, "#unique R" = 6, "Dulication Rate" = 0, "#C" = 2, "X" = 3, "Available Capacity" = 6,
    "Capacity Pressure" = 1, "Videos Per Cache"= 3, "V1Mo Rate" = 1, "Gini Total Video Per Request" = 0, "Median Endpoint Number Per Video" = 1,
    "Caches Per Weighted Request" = 1.5, "bestScoreRatio" = 0.6003, "Candidate Volume Per Cache" = 1.5
)

data_6970_311_100000_69 <- c(
    "GreedyTime" = 247.2, "#V" = 6970, "#E" = 311, "#R" = 100000, "#unique R" = 94583, "Dulication Rate" = 0.054, "#C" = 69, "X" = 200000, "Available Capacity" = 13800000,
    "Capacity Pressure" = 3.949, "Videos Per Cache"= 398.9, "V1Mo Rate" = 0.001, "Gini Total Video Per Request" = 0.38, "Median Endpoint Number Per Video" = 11,
    "Caches Per Weighted Request" = 10, "bestScoreRatio" = 0.9999, "Candidate Volume Per Cache" = 13.8
)

data_8_2_8_2 <- c(
    "GreedyTime" = 0, "#V" = 8, "#E" = 2, "#R" = 8, "#unique R" = 8, "Dulication Rate" = 0, "#C" = 2, "X" = 80, "Available Capacity" = 160,
    "Capacity Pressure" = 1.135, "Videos Per Cache" = 4.5, "V1Mo Rate" = 0.125, "Gini Total Video Per Request" = 0.12, "Median Endpoint Number Per Video" = 1,
    "Caches Per Weighted Request" = 1.4, "bestScoreRatio" = 0.5050, "Candidate Volume Per Cache" = 1.3
)

data_big <- c(
    "GreedyTime" = 147.1, "#V" = 6891, "#E" = 865, "#R" = 8762, "#unique R" = 8762, "Dulication Rate" = 0, "#C" = 500, "X" = 1000, "Available Capacity" = 500000,
    "Capacity Pressure" = 0.564, "Videos Per Cache"= 7.8, "V1Mo Rate" = 0, "Gini Total Video Per Request" = 0.49, "Median Endpoint Number Per Video" = 1,
    "Caches Per Weighted Request" = 1, "bestScoreRatio" = 0.9208, "Candidate Volume Per Cache" = 1.7
)

data_dense <- c(
    "GreedyTime" = 23.8, "#V" = 10000, "#E" = 500, "#R" = 100000, "#unique R" = 100000, "Dulication Rate" = 0, "#C" = 100, "X" = 1000, "Available Capacity" = 100000,
    "Capacity Pressure" = 0.022, "Videos Per Cache"= 2.2, "V1Mo Rate" = 0.1, "Gini Total Video Per Request" = 0.89, "Median Endpoint Number Per Video" = 10,
    "Caches Per Weighted Request" = 10, "bestScoreRatio" = 0.9203, "Candidate Volume Per Cache" = 2797
)

data_dense2 <- c(
    "GreedyTime" = 197.3, "#V" = 10000, "#E" = 500, "#R" = 100000, "#unique R" = 100000, "Dulication Rate" = 0, "#C" = 100, "X" = 100000, "Available Capacity" = 10000000,
    "Capacity Pressure" = 0.024, "Videos Per Cache"= 2.4, "V1Mo Rate" = 0.016, "Gini Total Video Per Request" = 0.81, "Median Endpoint Number Per Video" = 10,
    "Caches Per Weighted Request" = 10, "bestScoreRatio" = 0.1168, "Candidate Volume Per Cache" = 2575.8
)

data_INSATANC2D <- c(
    "GreedyTime" = 25.3, "#V" = 8000, "#E" = 110, "#R" = 399997, "#unique R" = 29458, "Dulication Rate" = 0.926, "#C" = 40, "X" = 3000, "Available Capacity" = 120000,
    "Capacity Pressure" = 0.250, "Videos Per Cache"= 48.9, "V1Mo Rate" = 0, "Gini Total Video Per Request" = 0.29, "Median Endpoint Number Per Video" = 4,
    "Caches Per Weighted Request" = 4, "bestScoreRatio" = 0.5267, "Candidate Volume Per Cache" = 50.6
)

data_INSTANCED <- c(
    "GreedyTime" = 0.3, "#V" = 600, "#E" = 100, "#R" = 37408, "#unique R" = 37408, "Dulication Rate" = 0, "#C" = 40, "X" = 8000, "Available Capacity" = 320000,
    "Capacity Pressure" = 0.630, "Videos Per Cache"= 9.5, "V1Mo Rate" = 0, "Gini Total Video Per Request" = 0.5, "Median Endpoint Number Per Video" = 63,
    "Caches Per Weighted Request" = 24.7, "bestScoreRatio" = 0.9914, "Candidate Volume Per Cache" = 63.5
)

data_medium_flat_dense_dejavu <- c(
    "GreedyTime" = 0.7, "#V" = 3000, "#E" = 80, "#R" = 640075, "#unique R" = 233021, "Dulication Rate" = 0.652, "#C" = 15, "X" = 2000, "Available Capacity" = 30000,
    "Capacity Pressure" = 0.177, "Videos Per Cache"= 35.3, "V1Mo Rate" = 0, "Gini Total Video Per Request" = 0.04, "Median Endpoint Number Per Video" = 75,
    "Caches Per Weighted Request" = 8, "bestScoreRatio" = 1, "Candidate Volume Per Cache" = 85
)

data_medium_flat_sparse_lambda <- c(
    "GreedyTime" = 0.5, "#V" = 3000, "#E" = 80, "#R" = 215637, "#unique R" = 215637, "Dulication Rate" = 0, "#C" = 15, "X" = 2000, "Available Capacity" = 30000,
    "Capacity Pressure" = 0.039, "Videos Per Cache"= 7.8, "V1Mo Rate" = 0, "Gini Total Video Per Request" = 0.02, "Median Endpoint Number Per Video" = 72,
    "Caches Per Weighted Request" = 3.2, "bestScoreRatio" = 1, "Candidate Volume Per Cache" = 385.3
)

data_medium_mixed_lambda <- c(
    "GreedyTime" = 0.3, "#V" = 3000, "#E" = 80, "#R" = 106790, "#unique R" = 106790, "Dulication Rate" = 0, "#C" = 15, "X" = 2000, "Available Capacity" = 30000,
    "Capacity Pressure" = 0.039, "Videos Per Cache"= 7.8, "V1Mo Rate" = 0, "Gini Total Video Per Request" = 0.38, "Median Endpoint Number Per Video" = 36,
    "Caches Per Weighted Request" = 7.2, "bestScoreRatio" = 1, "Candidate Volume Per Cache" = 384
)

data_medium_skew_dense_dejavu <- c(
    "GreedyTime" = 0.5, "#V" = 3000, "#E" = 80, "#R" = 640030, "#unique R" = 165599, "Dulication Rate" = 0.741, "#C" = 15, "X" = 2000, "Available Capacity" = 30000,
    "Capacity Pressure" = 0.180, "Videos Per Cache"= 35.2, "V1Mo Rate" = 0, "Gini Total Video Per Request" = 0.5, "Median Endpoint Number Per Video" = 70,
    "Caches Per Weighted Request" = 8, "bestScoreRatio" = 1, "Candidate Volume Per Cache" = 81.9
)

data_medium_skew_sparse_lambda <- c(
    "GreedyTime" = 0.3, "#V" = 3000, "#E" = 80, "#R" = 54235, "#unique R" = 54235, "Dulication Rate" = 0, "#C" = 15, "X" = 2000, "Available Capacity" = 30000,
    "Capacity Pressure" = 0.039, "Videos Per Cache"= 7.8, "V1Mo Rate" = 0, "Gini Total Video Per Request" = 0.61, "Median Endpoint Number Per Video" = 16,
    "Caches Per Weighted Request" = 3, "bestScoreRatio" = 1, "Candidate Volume Per Cache" = 368.8
)

data_realistic_large_clustered <- c(
    "GreedyTime" = 1569.5, "#V" = 10000, "#E" = 1000, "#R" = 200000, "#unique R" = 200000, "Dulication Rate" = 0, "#C" = 500, "X" = 10000, "Available Capacity" = 5000000,
    "Capacity Pressure" = 2.481, "Videos Per Cache"= 48.2, "V1Mo Rate" = 0, "Gini Total Video Per Request" = 0.77, "Median Endpoint Number Per Video" = 5,
    "Caches Per Weighted Request" = 59.2, "bestScoreRatio" = 0.9974, "Candidate Volume Per Cache" = 66.1
)

data_realistic_large_random <- c(
    "GreedyTime" = 2220.6, "#V" = 10000, "#E" = 1000, "#R" = 200000, "#unique R" = 200000, "Dulication Rate" = 0, "#C" = 500, "X" = 10000, "Available Capacity" = 5000000,
    "Capacity Pressure" = 2.448, "Videos Per Cache"= 47.7, "V1Mo Rate" = 0, "Gini Total Video Per Request" = 0.77, "Median Endpoint Number Per Video" = 5,
    "Caches Per Weighted Request" = 74.8, "bestScoreRatio" = 0.9997, "Candidate Volume Per Cache" = 121.2
)

data_regional_instance <- c(
    "GreedyTime" = 1.3, "#V" = 1000, "#E" = 100, "#R" = 1000, "#unique R" = 907, "Dulication Rate" = 0.093, "#C" = 250, "X" = 1800, "Available Capacity" = 450000,
    "Capacity Pressure" = 2.744, "Videos Per Cache"= 4.5, "V1Mo Rate" = 0, "Gini Total Video Per Request" = 0.54, "Median Endpoint Number Per Video" = 1,
    "Caches Per Weighted Request" = 3.2, "bestScoreRatio" = 0.9965, "Candidate Volume Per Cache" = 2
)

# For some reason, same as supra
#data_trap <- c(
#    "GreedyTime" = 1.7, "#V" = 1369, "#E" = 379, "#R" = 1984, "#unique R" = 1984, "Dulication Rate" = 0, "#C" = 200, "X" = 500, "Available Capacity" = 100000,
#    "Capacity Pressure" = 0.578, "Videos Per Cache"= 4, "V1Mo Rate" = 0, "Gini Total Video Per Request" = 0.43, "Median Endpoint Number Per Video" = 1,
#    "Caches Per Weighted Request" = 1
#)

data_supra <- c(
    "GreedyTime" = 2, "#V" = 1369, "#E" = 379, "#R" = 1984, "#unique R" = 1984, "Dulication Rate" = 0, "#C" = 200, "X" = 500, "Available Capacity" = 100000,
    "Capacity Pressure" = 0.578, "Videos Per Cache"= 4, "V1Mo Rate" = 0, "Gini Total Video Per Request" = 0.43, "Median Endpoint Number Per Video" = 1,
    "Caches Per Weighted Request" = 1, "bestScoreRatio" = 0.8977, "Candidate Volume Per Cache" = 1.7
)

data_zipF_instance <- c(
    "GreedyTime" = 1.3, "#V" = 1000, "#E" = 100, "#R" = 1000, "#unique R" = 899, "Dulication Rate" = 0.101, "#C" = 250, "X" = 1800, "Available Capacity" = 450000,
    "Capacity Pressure" = 3.321, "Videos Per Cache"= 4.5, "V1Mo Rate" = 0, "Gini Total Video Per Request" = 0.64, "Median Endpoint Number Per Video" = 1,
    "Caches Per Weighted Request" = 2.8, "bestScoreRatio" = 0.9918, "Candidate Volume Per Cache" = 1.8
)

##### Form the datasets ####
dataset_names <- ls()

df <- dplyr::bind_rows(list(
    data_10000_500_100000_100, 
    data_2_2_2_1,
    data_20_2_20_2,
    data_3860_246_84901_66,
    data_4_2_4_2,
    data_4990_246_84901_50,
    data_6_2_6_2, 
    data_6970_311_100000_69,       
    data_8_2_8_2,
    data_big,
    data_dejaVu42, 
    data_dense,                    
    data_dense2,
    data_INSATANC2D,
    data_INSTANCED,
    data_kittens,
    data_lambda42,
    data_meAtTheZoo,
    data_medium_flat_dense_dejavu,
    data_medium_flat_sparse_lambda,
    data_medium_mixed_lambda, 
    data_medium_skew_dense_dejavu,
    data_medium_skew_sparse_lambda,
    data_realistic_large_clustered,
    data_realistic_large_random,
    data_regional_instance,
    data_supra,
    data_trendingToday,
    data_videosWorthSpreading,     
    data_zipF_instance
))

# Remove cols of df for analysis
df <- df %>%
    select(-c(`Candidate Volume Per Cache`, `Caches Per Weighted Request`))

# Add rownames
rownames(df) <- dataset_names

#### Analysis ####
##### Kendall Correlation ####
# Correlation
df_corr <- cor(df, method = "kendall")

# Significance
df_corr.pval <- df_corr
df_corr.pval[,] <- 0

for (i in rownames(df_corr)) {
    for (j in colnames(df_corr)) {
        df_corr.pval[i,j] <- cor.test(unlist(df[,i]), unlist(df[,j]), method="kendall", exact=F)$p.value
    }
}

# Corrplot
df_corr2 <- df_corr
df_corr2[df_corr.pval>0.05] <- NA 
corrplot(df_corr2, tl.cex=0.5, method = "color")
View(df_corr)

###### Plot of Kendall Correlation ####
# Generate the dataframe (Time)
df_ggplot_corrTime <- bind_cols(df_corr["GreedyTime",], df_corr.pval["GreedyTime",]) %>%
    dplyr::rename(KendallCorr=`...1`, p.value = `...2`) %>%
    dplyr::mutate(AbsKendallCorr = abs(KendallCorr), significant = p.value<0.05)
df_ggplot_corrTime <- df_ggplot_corrTime %>%
    dplyr::mutate(Metric = factor(names(df_corr["GreedyTime",]))) %>%
    dplyr::arrange(desc(AbsKendallCorr))

# Plot Time
ggplot(df_ggplot_corrTime, aes(y=forcats::fct_reorder(Metric, AbsKendallCorr), x=AbsKendallCorr)) +
    geom_col(aes(fill = KendallCorr, colour = significant), lwd=1) +
    scale_fill_gradient2() + 
    theme(
        axis.text.x = element_text(size=7),
        axis.text.y = element_text(vjust = 0.5, hjust = 1, size=7),
        axis.title.y.left = element_blank(),
        legend.title.position = "left",
        legend.title = element_text(angle = 90, size=9),
        legend.text = element_text(size=7),
        legend.background = element_rect(fill = "gray90", colour = "black")
    ) +
    ggtitle("Corrélation de Kendall", subtitle = "Contre GreedyTime") +
    xlab("Corrélation de Kendall (VA)") +
    labs(fill = "Corrélation de Kendall", colour = "Significatif")

# Generate the dataframe (bestScore)
df_ggplot_corrScore <- bind_cols(df_corr["bestScoreRatio",], df_corr.pval["bestScoreRatio",]) %>%
    dplyr::rename(KendallCorr=`...1`, p.value = `...2`) %>%
    dplyr::mutate(AbsKendallCorr = abs(KendallCorr), significant = p.value<0.05) 
df_ggplot_corrScore <- df_ggplot_corrScore %>%
    dplyr::mutate(Metric = factor(names(df_corr["bestScoreRatio",]))) %>%
    dplyr::arrange(desc(AbsKendallCorr))

# Plot Time
ggplot(df_ggplot_corrScore, aes(y=forcats::fct_reorder(Metric, AbsKendallCorr), x=AbsKendallCorr)) +
    geom_col(aes(fill = KendallCorr, colour = significant), lwd=1) +
    scale_fill_gradient2() + 
    theme(
        axis.text.x = element_text(size=7),
        axis.text.y = element_text(vjust = 0.5, hjust = 1, size=7),
        axis.title.y.left = element_blank(),
        legend.title.position = "left",
        legend.title = element_text(angle = 90, size=9),
        legend.text = element_text(size=7),
        legend.background = element_rect(fill = "gray90", colour = "black")
    ) +
    ggtitle("Corrélation de Kendall", subtitle = "Contre bestScoreRatio") +
    xlab("Corrélation de Kendall (VA)") +
    labs(fill = "Corrélation de Kendall", colour = "Significatif")

##### PCA Analysis ####
dfPCA <- df
dfPCA$instance <- as.factor(rownames(dfPCA))
resultsPCA <- FactoMineR::PCA(dfPCA, quanti.sup = c("GreedyTime", "bestScoreRatio"), quali.sup = "instance")

# Plot for time
pGreedyTime <- factoextra::fviz_pca_ind(
    resultsPCA,
    col.ind = log(dfPCA$GreedyTime, 10),
    gradient.cols = muted(topo.colors(5),l=80, c=90),
    graph.type ="ggplot",
    label = "none",
    repel = TRUE
)

factoextra::fviz_add(
    pGreedyTime,
    resultsPCA$quali.sup$coord,
    geom = "text",
    color = "darkmagenta"
) +
    ggtitle("Analyse en Composante Principale des Métriques", subtitle = "Graphe des individus + GreedyTime") +
    theme(
        axis.text.x = element_text(size=7),
        axis.text.y = element_text(vjust = 0.5, hjust = 1, size=7),
        legend.title.position = "left",
        legend.title = element_text(angle = 90, size=9),
        legend.text = element_text(size=7),
        legend.background = element_rect(fill = "gray90", colour = "black")
    ) +
    labs(color = "Log10 GreedyTime")

# Plot for score
pScore <- factoextra::fviz_pca_ind(
    resultsPCA,
    col.ind = dfPCA$bestScoreRatio,
    gradient.cols = muted(topo.colors(5),l=80, c=90),
    graph.type ="ggplot",
    label = "none",
    repel = TRUE
)

factoextra::fviz_add(
    pScore,
    resultsPCA$quali.sup$coord,
    geom = "text",
    color = "darkmagenta"
) +
    ggtitle("Analyse en Composante Principale des Métriques", subtitle = "Graphe des individus + BestScoreRatio") +
    theme(
        axis.text.x = element_text(size=7),
        axis.text.y = element_text(vjust = 0.5, hjust = 1, size=7),
        legend.title.position = "left",
        legend.title = element_text(angle = 90, size=9),
        legend.text = element_text(size=7),
        legend.background = element_rect(fill = "gray90", colour = "black")
    ) +
    labs(color = "bestScoreRatio")