library(readxl)
library(dplyr)
library(ggplot2)

# General Hospital Data
file <- read_xlsx("hospital_data.xlsx")
hospital_data <- data.frame(file)

# Structure of the data
glimpse(hospital_data)

sorted_hospital_data <- hospital_data[order(hospital_data$Org.Dscr, decreasing = FALSE),]

clear_hospital_data <- na.omit(sorted_hospital_data)

sum_of_nursing_days <- aggregate(x=clear_hospital_data$Nursing.Day.Num,
                         by=list(clear_hospital_data$Org.Dscr),
                         FUN=sum)

sum_of_patients <- aggregate(x=clear_hospital_data$Patient.Num,
                         by=list(clear_hospital_data$Org.Dscr),
                         FUN=sum)

# Economics
financial_data <- read_xlsx("financial_data_hospitals.xlsx")
financial_data <- data.frame(financial_data)

sorted_financial_data <- financial_data[order(financial_data$HOSPITALS, decreasing = FALSE),]

expenditures_for_materials_drugs_services <- aggregate(x=sorted_financial_data$SUM.PURCHASING,
                                        by=list(sorted_financial_data$HOSPITALS),
                                        FUN=sum)
# Beds
hospital_beds <- read_xlsx("hospital_beds.xlsx")
hospital_beds <- data.frame(hospital_beds)

sorted_hospital_beds <- hospital_beds[order(hospital_beds$Hospitals, decreasing = FALSE),]

####### Data preprocessing #######
clear_hospital_beds <- na.omit(sorted_hospital_beds)

###### Economical pointers ######
sum_of_beds <- aggregate(x=clear_hospital_beds$Beds,
                                 by=list(clear_hospital_beds$Hospitals),
                                 FUN=sum)

data <- data.frame(sum_of_patients$Group.1, financial_data$YPE, sum_of_patients$x, sum_of_nursing_days$x,expenditures_for_materials_drugs_services$x)
# First economical pointer

# Average
average_total_cost_per_patient <- (data$expenditures_for_materials_drugs_services.x) / (data$sum_of_patients.x)
average_total_cost_per_nursing_days <- (data$expenditures_for_materials_drugs_services.x) / (data$sum_of_nursing_days.x)

# Regression Analysis
ra_first_1 = lm(data$expenditures_for_materials_drugs_services.x~data$sum_of_patients.x)
summary(ra_first_1)

# Plot the chart.
plot(data$sum_of_patients.x,data$expenditures_for_materials_drugs_services.x,col = "blue",main = "Correlation of Average Total Cost with the number of hospitalized patients",
     abline(ra_first_1),cex = 1.3,pch = 16,xlab = "Hospitalized Patients",ylab = "Total Cost")

ra_first_2 = lm(data$expenditures_for_materials_drugs_services.x~data$sum_of_nursing_days.x)
summary(ra_first_2)

plot(data$sum_of_nursing_days.x,data$expenditures_for_materials_drugs_services.x,col = "blue",main = "Correlation of Average Total Cost with the number of hospitalized patients days",
     abline(ra_first_2),cex = 1.3,pch = 16,xlab = "Hospitalized Patient Days",ylab = "Total Cost")

# Plot
graph <- ggplot(data=data, aes(x=average_total_cost_per_patient, y=expenditures_for_materials_drugs_services$Group.1)) +
  geom_bar(stat="identity")
graph

# Second economical pointer
expenditures_for_consumptions <- aggregate(x=sorted_financial_data$SUM.CONSUMAMBLES,
                                        by=list(sorted_financial_data$HOSPITALS),
                                        FUN=sum)

data <- data.frame(sum_of_patients$Group.1, financial_data$YPE, sum_of_patients$x, sum_of_nursing_days$x,expenditures_for_materials_drugs_services$x,expenditures_for_consumptions$x)

# Average
average_total_cost_materials_consumables_per_patient <- (data$expenditures_for_consumptions.x) / (data$sum_of_patients.x)
average_total_cost_materials_consumamble_per_nursing_days <- (data$expenditures_for_consumptions.x) / (data$sum_of_nursing_days.x)

# Regression Analysis
ra_second_1 = lm(data$expenditures_for_consumptions.x~data$sum_of_patients.x)
summary(ra_second_1)

# Plot the chart.
plot(data$sum_of_patients.x,data$expenditures_for_consumptions.x,col = "blue",main = "Correlation of Cost for Materials & Consumambles per patient",
     abline(ra_second_1),cex = 1.3,pch = 16,xlab = "Hospitalized Patients",ylab = "Materials and Consumptions Cost")

ra_second_2 = lm(data$expenditures_for_consumptions.x~data$sum_of_nursing_days.x)
summary(ra_second_2)

plot(data$sum_of_nursing_days.x,data$expenditures_for_consumptions.x,col = "blue",main = "Correlation of Cost for Materials & Consumambles per patient day",
     abline(ra_second_2),cex = 1.3,pch = 16,xlab = "Hospitalized Patient Day",ylab = "Materials and Consumptions Cost")

# Plot
graph_2 <- ggplot(data=data, aes(x=average_total_cost_materials_consumables_per_patient, y=expenditures_for_consumptions$Group.1)) +
  geom_bar(stat="identity")
graph_2

# Third economical pointer
expenditures_for_drugs <- aggregate(x=sorted_financial_data$DRUGS,
                                           by=list(sorted_financial_data$HOSPITALS),
                                           FUN=sum)

data <- data.frame(sum_of_patients$Group.1, financial_data$YPE, sum_of_patients$x, sum_of_nursing_days$x,expenditures_for_materials_drugs_services$x,expenditures_for_consumptions$x, expenditures_for_drugs$x)

# Average
pharmaceutical_cost_per_patient <- (data$expenditures_for_drugs.x) / (data$sum_of_patients.x)
pharmaceutical_total_cost_per_nursing_days <- (data$expenditures_for_drugs.x) / (data$sum_of_nursing_days.x)

# Regression Analysis
ra_third_1 = lm(data$expenditures_for_drugs.x~data$sum_of_patients.x)
summary(ra_third_1)

# Plot the chart.
plot(data$sum_of_patients.x,data$expenditures_for_drugs.x,col = "blue",main = "Correlation of Pharmaceutical Cost with the number of patients",
     abline(ra_third_1),cex = 1.3,pch = 16,xlab = "Hospitalized Patients",ylab = "Pharmaceutical Cost")

ra_third_2 = lm(data$expenditures_for_drugs.x~data$sum_of_nursing_days.x)
summary(ra_third_2)

plot(data$sum_of_nursing_days.x,data$expenditures_for_drugs.x,col = "blue",main = "Correlation of Pharmaceutical Cost with the number of patient days",
     abline(ra_third_2),cex = 1.3,pch = 16,xlab = "Hospitalized Patient Days",ylab = "Pharmaceutical Cost")

# Plot
graph_3 <- ggplot(data=data, aes(x=pharmaceutical_cost_per_patient, y=expenditures_for_drugs$Group.1)) +
  geom_bar(stat="identity")
graph_3

# Fourth economical pointer
expenditures_for_services <- aggregate(x=sorted_financial_data$SERVICES,
                                    by=list(sorted_financial_data$HOSPITALS),
                                    FUN=sum)

data <- data.frame(sum_of_patients$Group.1, financial_data$YPE, sum_of_patients$x, sum_of_nursing_days$x, sum_of_beds$x, expenditures_for_materials_drugs_services$x,expenditures_for_consumptions$x, expenditures_for_drugs$x
                  ,expenditures_for_services$x)

# Average
average_cost_services_per_patient <- (data$expenditures_for_services.x) / (data$sum_of_patients.x)
average_cost_services_per_nursing_days <- (data$expenditures_for_services.x) / (data$sum_of_nursing_days.x)

# Regression Analysis
ra_fourth_1 = lm(data$expenditures_for_services.x~data$sum_of_patients.x)
summary(ra_fourth_1)

# Plot the chart.
plot(data$sum_of_patients.x,data$expenditures_for_services.x,col = "blue",main = "Correlation of Services Cost with the number of patients",
     abline(ra_fourth_1),cex = 1.3,pch = 16,xlab = "Hospitalized Patients",ylab = "Services Cost")

ra_fourth_2 = lm(data$expenditures_for_services.x~data$sum_of_nursing_days.x)
summary(ra_fourth_2)

plot(data$sum_of_nursing_days.x,data$expenditures_for_services.x,col = "blue",main = "Correlation of Services  Cost with the number of patient days",
     abline(ra_fourth_2),cex = 1.3,pch = 16,xlab = "Hospitalized Patient Days",ylab = "Services Cost")

# Plot
graph_4 <- ggplot(data=data, aes(x=average_cost_services_per_patient, y=expenditures_for_services$Group.1)) +
  geom_bar(stat="identity")
graph_4

# Fifth economical pointer
expenditures_for_outsourcing_service <- aggregate(x=sorted_financial_data$SERVICES,
                                       by=list(sorted_financial_data$HOSPITALS),
                                       FUN=sum)

data <- data.frame(sum_of_patients$Group.1, financial_data$YPE, sum_of_patients$x, sum_of_nursing_days$x,sum_of_beds$x,expenditures_for_materials_drugs_services$x,expenditures_for_consumptions$x, expenditures_for_drugs$x
                   ,expenditures_for_services$x, expenditures_for_outsourcing_service$x)

# Average
average_monthly_cost_out_services_per_bed <- ((data$expenditures_for_outsourcing_service.x)/12) / (data$sum_of_beds.x)
average_outsourcing_cost_services_per_patient <- (data$expenditures_for_outsourcing_service.x) / (data$sum_of_patients.x)

# Regression Analysis
ra_fifth_1 = lm(data$expenditures_for_outsourcing_service.x~data$sum_of_beds.x)
summary(ra_fifth_1)

# Plot the chart.
plot(data$sum_of_beds.x,data$expenditures_for_outsourcing_service.x,col = "blue",main = "Correlation of Outsourcing Service Cost per month with the number of beds",
     abline(ra_fifth_1),cex = 1.3,pch = 16,xlab = "Beds",ylab = "Monthly Outsourcing Service Cost")

ra_fifth_2 = lm(data$expenditures_for_outsourcing_service.x~data$sum_of_patients.x)
summary(ra_fifth_2)

plot(data$sum_of_patients.x,data$expenditures_for_outsourcing_service.x,col = "blue",main = "Correlation of Outsourcing Service Cost with the number of patients",
     abline(ra_fifth_2),cex = 1.3,pch = 16,xlab = "Hospitalized Patients",ylab = "Outsourcing Service Cost")

# Plot
graph_5 <- ggplot(data=data, aes(x=average_outsourcing_cost_services_per_patient, y=expenditures_for_outsourcing_service$Group.1)) +
  geom_bar(stat="identity")
graph_5

######## Operative pointers ########
# First operative pointer
# Average
average_length_of_stay <- (data$sum_of_nursing_days.x) / (data$sum_of_patients.x)

# Plot
graph_7 <- ggplot(data=data, aes(x=average_length_of_stay, y=expenditures_for_outsourcing_service$Group.1)) +
  geom_bar(stat="identity")
graph_7

# Second operative pointer
# Average
bed_occupancy <- ((data$sum_of_nursing_days.x) / (data$sum_of_beds.x * 365)) * 100

# Plot
graph_8 <- ggplot(data=data, aes(x=bed_occupancy, y=expenditures_for_outsourcing_service$Group.1)) +
  geom_bar(stat="identity")
graph_8