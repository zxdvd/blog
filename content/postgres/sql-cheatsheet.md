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

<details open><summary>postgres</summary>

* base64 decode/encode

      select encode('hello', 'base64');    -- output aGVsbG8=
      select convert_from(decode('aGVsbG8=', 'base64'), 'utf8');  -- decode returns bytea
</details>
<details><summary>mysql</summary>

* base64 decode/encode

      select to_base64('hello');    -- output aGVsbG8=
      select from_base64('aGVsbG8=');
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
