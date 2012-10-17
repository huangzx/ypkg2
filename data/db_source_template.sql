CREATE TABLE  IF NOT EXISTS additional_xxx_yyy(name TEXT, generic_name TEXT, is_desktop INTEGER, category TEXT, arch TEXT, version TEXT, exec TEXT, rir INTEGER, priority TEXT, install TEXT, license TEXT, homepage TEXT, repo TEXT, size INTEGER, sha TEXT, build_date INTEGER, packager TEXT, uri TEXT, description TEXT,installed INTEGER DEFAULT 0, can_update INTEGER DEFAULT 0, data_count INTEGER, PRIMARY KEY(name));

CREATE TABLE  IF NOT EXISTS additional_xxx_yyy_data (name TEXT, version TEXT, data_name TEXT, data_format TEXT, data_size INTEGER, data_install_size INTEGER, data_depend TEXT, data_bdepend TEXT, data_recommended TEXT, data_conflict TEXT, data_replace TEXT);

CREATE INDEX  IF NOT EXISTS additional_xxx_yyy_data_name  ON additional_xxx_yyy_data ( name );

REPLACE INTO source VALUES( NULL, 'xxx', 'yyy', 0, 0, 1 );
