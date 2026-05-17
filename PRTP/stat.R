sensor_dir <- "sensor_log"
server_dir <- "server_sensor_log"
dirs <- dir(pattern = "*_log$")
tmp = unlist(strsplit(dirs, split="_"))
clients <- tmp[(tmp != "sensor") & (tmp != "server") & (tmp != "log")]
tmp = unlist(strsplit(dir(sensor_dir, pattern = "*.log"), split="\\."))
sensors <- tmp[seq(1, length(tmp), 2)]

for(s in sensors) {
  cat("Sensor", s, "\n")
  tmp = file.path(sensor_dir, paste(s, "log", sep="."))
  sensor_side <- read.table(tmp)
  tmp = file.path(server_dir, paste(s, "log", sep="."))
  server_side <- read.table(tmp)
  sensor_total <- length(sensor_side$V2)
  server_total <- length(sensor_side$V2)
  cat("Server got", server_total, "out of", sensor_total, "\n")

  time_start = min(server_side$V1)+1 #add this to allow sensor to generate smth
  time_stop = max(server_side$V1)
  time_range = time_start:time_stop
  
  for(c in clients) {
    cat("client", c , "\n");
    client_dir = paste(c, sensor_dir, sep="_")
    tmp = file.path(client_dir, paste(s, "log", sep="."))
    client_side <- read.table(tmp)
    skew <- c()
    
    lost <- setdiff(server_side$V2, client_side$V2)
    cat(c, "lost", length(lost), "out of", server_total, "\n")
    dup_client <- duplicated(client_side$V2)
    cat(c, "duplicated", sum(dup_client), "\n")
    cat(c, "out of order", sum(client_side$V2 > cummax(client_side$V2)), "\n")
    delivered <- server_side$V2 %in% client_side$V2[!dup_client]
    sent <- client_side$V2[!dup_client] %in% server_side$V2
    delays <- client_side$V1[!dup_client][sent]-server_side$V1[delivered]
    cat(" mean delay", mean(delays), "\n")
    cat(" max delay", max(delays), "\n")
    cat(" min delay", min(delays), "\n")
    cat(" variance delay", var(delays), "\n")

    for(t in time_range) {
      sensor_slice <- max(sensor_side$V2[sensor_side$V1 < t])
      client_slice <- max(client_side$V2[client_side$V1 < t])
      skew <- c(skew, sensor_slice - client_slice)
    }

    cat(" mean skew", mean(skew), "\n")
    cat(" max skew", max(skew), "\n")
    cat(" min skew", min(skew), "\n")
    cat(" variance skew", var(skew), "\n")

  }
  cat("-------------------------\n")
}
