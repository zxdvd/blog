### Frequent used sql queries

|   |postgres|mysql|
|---|  ---   | --- |
| server version | select version() | select version() |
| show timezone | show timezone<br>select current_setting('timezone') | select  @@session.time_zone |
| timezone offset | select extract(timezone from now())/3600 |  |
| interval to seconds  | select extract(epoch from interval '1 hours') |  |
| timestamp to seconds | select extract(epoch from now()) | select to_seconds(now()) |
|  |  |  |
