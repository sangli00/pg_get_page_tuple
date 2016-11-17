\echo Use "create extension pg_fuck_block" to load this file. \quit

CREATE FUNCTION pg_get_page_tuple(regclass , bigint)
   RETURNS SETOF RECORD
   AS 'MODULE_PATHNAME', 'pg_get_page_tuple'
  LANGUAGE C STRICT;
