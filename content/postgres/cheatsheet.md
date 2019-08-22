### Frequent used sql queries

#### show server version
<details open>
  <summary>postgres</summary>
      
    select version();
</details>
<details>
  <summary>mysql</summary>
      
    select version();
</details>
 
#### timezone related
* show timezone
  <details open>
    <summary>postgres</summary>

      show timezone;
      select current_setting('timezone');
  </details>
  <details>
    <summary>mysql</summary>

      select @@session.time_zone;
  </details>

* get timezone offset
  <details open>
    <summary>postgres</summary>

      select extract(timezone from now())/3600;
  </details>
  <details>
    <summary>mysql</summary>

      select @@session.time_zone;
  </details>

#### datetime related
* interval to seconds
  <details open>
    <summary>postgres</summary>

      select extract(epoch from interval '1 hours');
  </details>
  <details>
    <summary>mysql</summary>

      --select @@session.time_zone;
  </details>
* timestamp to seconds
  <details open>
    <summary>postgres</summary>

      select extract(epoch from now());
  </details>
  <details>
    <summary>mysql</summary>

      select to_seconds(now());
  </details>
