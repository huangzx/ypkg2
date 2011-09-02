CREATE TABLE alias (name TEXT, alias TEXT);
CREATE TABLE config (id INTEGER PRIMARY KEY, last_update INTEGER, has_new INTEGER, last_check INTEGER);
CREATE TABLE universe (name TEXT PRIMARY KEY, generic_name TEXT, is_desktop INTEGER, category TEXT, arch TEXT, version TEXT, priority TEXT, install TEXT, license TEXT, homepage TEXT, repo TEXT, size INTEGER, sha TEXT, build_date INTEGER, uri TEXT, description TEXT,installed INTEGER DEFAULT 0, data_count INTEGER);
CREATE TABLE universe_data (name TEXT, data_name TEXT, data_format TEXT, data_size INTEGER, data_install_size INTEGER, data_depend TEXT, data_bdepend TEXT, data_recommended TEXT, data_conflict TEXT);
CREATE TABLE universe_language (name TEXT, language TEXT,  generic_name TEXT, description TEXT );
CREATE TABLE world (name TEXT PRIMARY KEY, generic_name TEXT, is_desktop INTEGER, category TEXT, arch TEXT, version TEXT, priority TEXT, install TEXT, license TEXT, homepage TEXT, repo TEXT, size INTEGER, sha TEXT, build_date INTEGER, uri TEXT, description TEXT, can_update INTEGER DEFAULT 0 , data_count INTEGER, install_time INTEGER);
CREATE TABLE world_data (name TEXT, data_name TEXT, data_format TEXT, data_size INTEGER, data_install_size INTEGER, data_depend TEXT, data_bdepend TEXT, data_recommended TEXT, data_conflict TEXT);
CREATE TABLE world_file (name TEXT, file TEXT , type TEXT, size INTEGER, perms TEXT, uid INTEGER, gid INTEGER, mtime INTEGER);
CREATE TABLE world_language (name TEXT, language TEXT,  generic_name TEXT, description TEXT );
CREATE INDEX universe_data_name ON universe_data ( name );
CREATE INDEX universe_language_language ON universe_language ( language );
CREATE INDEX universe_language_name ON universe_language ( name );
CREATE INDEX world_data_name ON world_data ( name );
CREATE INDEX world_file_name ON world_file ( name );
CREATE INDEX world_language_language ON world_language ( language );
CREATE INDEX world_language_name ON world_language ( name );
CREATE INDEX world_name ON world ( name );
