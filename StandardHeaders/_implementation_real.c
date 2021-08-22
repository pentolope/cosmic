
#include <stdint.h>
#include <stdbool.h>

#define FAT16_CLUSTER_FREE         0x0000
#define FAT16_CLUSTER_RESERVED_MIN 0xfff0
#define FAT16_CLUSTER_RESERVED_MAX 0xfff6
#define FAT16_CLUSTER_BAD          0xfff7
#define FAT16_CLUSTER_LAST_MIN     0xfff8
#define FAT16_CLUSTER_LAST_MAX     0xffff

#define FAT32_CLUSTER_FREE         0x00000000
#define FAT32_CLUSTER_RESERVED_MIN 0x0ffffff0
#define FAT32_CLUSTER_RESERVED_MAX 0x0ffffff6
#define FAT32_CLUSTER_BAD          0x0ffffff7
#define FAT32_CLUSTER_LAST_MIN     0x0ffffff8
#define FAT32_CLUSTER_LAST_MAX     0x0fffffff

#define FAT_DIRENTRY_DELETED 0xe5

#define FAT_ATTRIB_READONLY (1 << 0)
#define FAT_ATTRIB_HIDDEN   (1 << 1)
#define FAT_ATTRIB_SYSTEM   (1 << 2)
#define FAT_ATTRIB_VOLUME   (1 << 3)
#define FAT_ATTRIB_DIR      (1 << 4)
#define FAT_ATTRIB_ARCHIVE  (1 << 5)


uint16_t read2(const uint8_t* p){
	return ((uint16_t)p[0] << 0) | ((uint16_t)p[1] << 8);
}
uint32_t read4(const uint8_t* p){
	return ((uint32_t)p[0] << 0) | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
void write2(uint8_t* p, uint16_t v){
	p[0]=v >> 0;p[1]=v >> 8;
}
void write4(uint8_t* p, uint32_t v){
	p[0]=v >> 0;p[1]=v >> 8;p[2]=v >>16;p[3]=v >>24;
}

// struct Folder_File_Object represents a folder that is targetting a subfolder or file contained inside
struct Folder_File_Object {
	// struct Target_Folder_File represents the target of struct Folder_File_Object
	struct Target_Folder_File {
		uint8_t attributes;
		uint8_t checksum; // checksum is mainly used when generating data for this struct
		uint8_t written_entries_count; // is 0 if no directory entry is written for target in parent folder
		bool is_open_for_writing;
		uint32_t file_size; // is 0 for folders
		uint32_t directory_entry_cluster; // data cluster that holds the directory entry for the target in the parent folder
		uint32_t directory_entry_offset; // is in units of bytes. the offset is at the start of directory entries (LFN makes multiple entries, the offset and cluster point to the first one)
		uint32_t entrance_cluster; // the first cluster that holds the data for target folder/file
		uint32_t walking_cluster; // for the current walking position inside target folder/file
		uint32_t walking_position; // the current walking position inside target folder/file
		uint8_t name[256];
	} target;
	uint32_t parent_folder_entrance_cluster; // used primarily when writing a directory entry
	bool is_target_root; // if this is true then what struct Folder_File_Object represents is actually just the root directory of the filesystem. In which case, non of the other information is relevent.
};

struct File_System {
	bool isFAT16;
	uint8_t sectors_per_cluster; // the number of sectors in one cluster

	uint32_t root_dir_cluster; // root_dir_cluster is in units of clusters
	uint32_t data_cluster_count; // data_cluster_count is in units of clusters
	
	uint32_t fat_offset; // fat_offset is in units of sectors
	uint32_t cluster_zero_offset; // cluster_zero_offset is in units of sectors
	uint32_t root_dir_offset; // root_dir_offset is in units of sectors
	
	uint32_t fat16_root_max; // fat16_root_max is in units of bytes and is only applicable for fat16
	uint32_t cluster_size; // cluster_size is in units of bytes
} file_system = {0};

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CACHE_SIZE 15
#define CACHE_SECTOR_DATA_ADDRESS(index,offset) ((uint8_t*)((unsigned long)(index)*512lu+((0x80000000lu|0x1000000lu|512lu)+(unsigned long)(offset))))

struct Cache {
	uint8_t sector_count;
	uint8_t sector_walk;
	bool dirty[CACHE_SIZE];
	uint32_t sector_addresses[CACHE_SIZE];
} cache={0};


bool cache_to_card(uint8_t index){
	*((volatile uint8_t*)(0x80000000lu|0x03lu))=1;
	if (*((volatile uint8_t*)(0x80000000lu|0x1000000lu|0x04lu))!=4) goto Fail;
	*((volatile uint8_t*)(0x80000000lu|0x1000000lu|0x00lu))=1;
	*((volatile uint32_t*)(0x80000000lu|0x1000000lu|0x0Clu))=cache.sector_addresses[index];
	*((volatile uint8_t*)(0x80000000lu|0x1000000lu|0x08lu))=(index+1) & 15;
	*((volatile uint8_t*)(0x80000000lu|0x1000000lu|0x02lu))=1;
	while (1){
		uint8_t c=*((volatile uint8_t*)(0x80000000lu|0x1000000lu|0x04lu));
		if (c==3) break;
		if (c==0 | c==5 | c==6) goto Fail;
	}
	*((volatile uint8_t*)(0x80000000lu|0x1000000lu|0x02lu))=0;
	while (1){
		uint8_t c=*((volatile uint8_t*)(0x80000000lu|0x1000000lu|0x04lu));
		if (c==4) break;
		if (c!=3) goto Fail;
	}
	if (*((volatile uint16_t*)(0x80000000lu|0x1000000lu|0x06lu))!=0) goto Fail;
	*((volatile uint8_t*)(0x80000000lu|0x03lu))=0;
	return 0;
	Fail:;
	*((volatile uint8_t*)(0x80000000lu|0x04lu))=1;
	return 1;
}

bool cache_flush_single(uint8_t index){
	bool b=0;
	if (cache.dirty[index]){
		b=cache_to_card(index);
		cache.dirty[index]=0;
	}
	return b;
}

bool cache_flush_all(){
	for (uint8_t i=0;i<cache.sector_count;i++){
		if (cache_flush_single(i)) return 1;
	}
	return 0;
}

bool card_to_cache(uint8_t index){
	*((volatile uint8_t*)(0x80000000lu|0x02lu))=1;
	if (*((volatile uint8_t*)(0x80000000lu|0x1000000lu|0x04lu))!=4) goto Fail;
	*((volatile uint8_t*)(0x80000000lu|0x1000000lu|0x00lu))=0;
	*((volatile uint32_t*)(0x80000000lu|0x1000000lu|0x0Clu))=cache.sector_addresses[index];
	*((volatile uint8_t*)(0x80000000lu|0x1000000lu|0x08lu))=(index+1) & 15;
	*((volatile uint8_t*)(0x80000000lu|0x1000000lu|0x02lu))=1;
	while (1){
		uint8_t c=*((volatile uint8_t*)(0x80000000lu|0x1000000lu|0x04lu));
		if (c==3) break;
		if (c==0 | c==5 | c==6) goto Fail;
	}
	*((volatile uint8_t*)(0x80000000lu|0x1000000lu|0x02lu))=0;
	while (1){
		uint8_t c=*((volatile uint8_t*)(0x80000000lu|0x1000000lu|0x04lu));
		if (c==4) break;
		if (c!=3) goto Fail;
	}
	if (*((volatile uint16_t*)(0x80000000lu|0x1000000lu|0x06lu))!=0) goto Fail;
	*((volatile uint8_t*)(0x80000000lu|0x02lu))=0;
	return 0;
	Fail:;
	*((volatile uint8_t*)(0x80000000lu|0x04lu))=1;
	return 1;
}


// only needs to be called when that sector is not present in the cache
bool cache_load(uint32_t sector){
	uint8_t index;
	for (index=0;index<cache.sector_count;index++){
		if (cache.sector_addresses[index]==sector){
			return 0;
		}
	}
	if (cache.sector_count<CACHE_SIZE){
		index=cache.sector_count++;
	} else {
		index=cache.sector_walk;
		if (cache_flush_single(index)) return 1;
		if (++cache.sector_walk>=CACHE_SIZE){
			cache.sector_walk=0;
		}
	}
	cache.dirty[index]=0;
	cache.sector_addresses[index]=sector;
	bool b=card_to_cache(index);
	return b;
}

// via_sector is for a 32bit sector number with a 16bit byte offset. Nothing else is added to the address
bool cache_read_via_sector(uint32_t sector,uint16_t offset,uint32_t size,uint8_t* buffer){
	if (size==0) return 0;
	if (offset>=512u){
		sector+=(unsigned)offset >> 9;
		offset =(unsigned)offset & 511;
	}
	while (1){
		lstart:;
		for (uint8_t i=0;i<cache.sector_count;i++){
			if (sector==cache.sector_addresses[i]){
				uint32_t s=512u-offset;
				if (size<s) s=size;
				memcpy(buffer,CACHE_SECTOR_DATA_ADDRESS(i,offset),s);
				size-=s;
				if (size==0) return 0;
				buffer+=s;
				offset=0;
				sector++;
				goto lstart;
			}
		}
		if (cache_load(sector)) return 1;
	}
}

bool cache_write_via_sector(uint32_t sector,uint16_t offset,uint32_t size,const uint8_t* buffer){
	if (size==0) return 0;
	if (offset>=512u){
		sector+=(unsigned)offset >> 9;
		offset =(unsigned)offset & 511;
	}
	while (1){
		lstart:;
		for (uint8_t i=0;i<cache.sector_count;i++){
			if (sector==cache.sector_addresses[i]){
				cache.dirty[i]=1;
				uint32_t s=512u-offset;
				if (size<s) s=size;
				memcpy(CACHE_SECTOR_DATA_ADDRESS(i,offset),buffer,s);
				size-=s;
				if (size==0) return 0;
				buffer+=s;
				offset=0;
				sector++;
				goto lstart;
			}
		}
		if (cache_load(sector)) return 1;
	}
}

// at_cluster is for a 28bit cluster number with a 32bit byte offset. The header and table area is added to the address, thus it accesses the data of the clusters.
// if the file system is FAT16 and the cluster number is 0, then the root directory table is accessed instead.
// if the file system is FAT32 then the cluster number should never be 0.
// the cluster number should never be 1.
// the conditions stated above are not checked for validity.
bool cache_read_at_cluster(uint32_t cluster,uint32_t offset_large,uint32_t size,uint8_t* buffer){
	return cache_read_via_sector(
		((file_system.isFAT16 & cluster==0)?file_system.root_dir_offset:((cluster - 2) * file_system.sectors_per_cluster + file_system.cluster_zero_offset)) + (offset_large >> 9),
		(uint16_t)(offset_large & 511),
		size,
		buffer
	);
}

bool cache_write_at_cluster(uint32_t cluster,uint32_t offset_large,uint32_t size,const uint8_t* buffer){
	return cache_write_via_sector(
		((file_system.isFAT16 & cluster==0)?file_system.root_dir_offset:((cluster - 2) * file_system.sectors_per_cluster + file_system.cluster_zero_offset)) + (offset_large >> 9),
		(uint16_t)(offset_large & 511),
		size,
		buffer
	);
}

/*
at_table is for a 28bit cluster number. The header area is added to the address, thus it accesses the file allocation table.
no size is given because the size can be assumed.
additionally, read_at_table will set the following bits:
bit 28 if the value is "free cluster"
bit 29 if the value is "reserved cluster"
bit 30 if the value is "bad cluster"
bit 31 if the value is "last cluster"
*/
bool cache_read_at_table(uint32_t cluster,uint32_t* value){
	uint8_t buffer[4] = {0};
	const uint8_t size=file_system.isFAT16?2:4;
	cluster*=size;
	const uint32_t sector=file_system.fat_offset + (cluster >> 9);
	const uint16_t offset=cluster & 511;
	if (cache_read_via_sector(sector,offset,size,buffer)) return 1;
	// for fat32 the upper 4 bits of a cluster entry should be interpreted as 0
	// this transformation can be done when fat16 as well, because it doesn't make a difference for fat16
	buffer[3] &= 0x0f;
	uint32_t value_local=read4(buffer);
	value_local |= 
		((uint32_t)
			(
				(
					file_system.isFAT16
				)?(
					0x10*((uint16_t)value_local==FAT16_CLUSTER_FREE) |
					0x20*((uint16_t)value_local>=FAT16_CLUSTER_RESERVED_MIN & (uint16_t)value_local<=FAT16_CLUSTER_RESERVED_MAX) |
					0x40*((uint16_t)value_local==FAT16_CLUSTER_BAD) |
					0x80*((uint16_t)value_local>=FAT16_CLUSTER_LAST_MIN & (uint16_t)value_local<=FAT16_CLUSTER_LAST_MAX)
				):(
					0x10*(value_local==FAT32_CLUSTER_FREE) |
					0x20*(value_local>=FAT32_CLUSTER_RESERVED_MIN & value_local<=FAT32_CLUSTER_RESERVED_MAX) |
					0x40*(value_local==FAT32_CLUSTER_BAD) |
					0x80*(value_local>=FAT32_CLUSTER_LAST_MIN & value_local<=FAT32_CLUSTER_LAST_MAX)
				)
			)
		) << 24;
	*value=value_local;
	return 0;
}

bool cache_write_at_table(uint32_t cluster,uint32_t value){
	uint8_t buffer[4];
	write4(buffer,value);
	const uint8_t size=file_system.isFAT16?2:4;
	cluster*=size;
	const uint32_t sector=file_system.fat_offset + (cluster >> 9);
	const uint16_t offset=cluster & 511;
	if (!file_system.isFAT16){
		// for fat32 the upper 4 bits of a cluster entry should not be modified when writing the entry
		// this transformation is not done for fat16 because it is not desired
		uint8_t upper_byte=0;
		if (cache_read_via_sector(sector,offset+3,1,&upper_byte)) return 1;
		buffer[3]=(buffer[3] & 0x0f) | (upper_byte & 0xf0);
	}
	return cache_write_via_sector(sector,offset,size,buffer);
}



// ptr_cluster is read to get the current cluster and written with the value of the next cluster, or is written with 0 if there is no next cluster.
// for fat16, ptr_cluster should still be to a 32bit number, but the upper word will be set to 0
// returns 1 on IO error. Whenever it returns 1, ptr_cluster will have been set to 0
bool fat_get_next_cluster(uint32_t* ptr_cluster){
	uint32_t cluster=*ptr_cluster;
	*ptr_cluster=0;
	if (cluster < 2) return 0;
	if (cache_read_at_table(cluster,&cluster)) return 1;
	if ((cluster & 0xf0000000)!=0) return 0;
	*ptr_cluster=cluster;
	return 0;
}

uint8_t fat_calc_8_3_checksum(const uint8_t* file_name){
	uint8_t checksum=file_name[0];
	for (uint16_t i=1;i<11;i++) checksum = ((checksum >> 1) | (checksum << 7)) + file_name[i];
	return checksum;
}

// returns 0 if the next entry needs to be read, returns 1 if a subfolder/file has finished being read
bool fat_interpret_directory_entry(uint32_t current_cluster,uint32_t offset,const uint8_t* buffer,struct Folder_File_Object* ffo){
	if (buffer[0]==FAT_DIRENTRY_DELETED | buffer[0]==0){
		ffo->target.written_entries_count=0;
		return 0;
	}
	uint8_t* name=ffo->target.name;
	if (buffer[11]==0x0f){
		if (ffo->target.written_entries_count==0 | ffo->target.checksum!=buffer[13]){
			memset(&ffo->target,0,sizeof(struct Target_Folder_File));
			ffo->target.directory_entry_cluster=current_cluster;
			ffo->target.directory_entry_offset=offset;
			ffo->target.checksum=buffer[13];
		}
		ffo->target.written_entries_count+=1;
		const uint8_t char_mapping[13]={ 1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30 };
		const uint16_t char_offset = ((buffer[0] & 0x1f) - 1) * 13;
		for (uint16_t i=0;i<13;i++){
			uint16_t k=char_offset+i;
			if (k<255) name[k]=buffer[char_mapping[i]];
		}
		return 0;
	} else {
		if (fat_calc_8_3_checksum(buffer)!=ffo->target.checksum | ffo->target.written_entries_count==0 | name[0]==0){
			memset(&ffo->target,0,sizeof(struct Target_Folder_File));
			ffo->target.directory_entry_cluster=current_cluster;
			ffo->target.directory_entry_offset=offset;
			ffo->target.checksum=fat_calc_8_3_checksum(buffer);
			bool change_capitalization=(buffer[12] & 0x08)!=0;
			uint16_t i=0;
			while (i<8){
				const uint8_t c=buffer[i];
				if (c==' ') break;
				name[i++]=c+(change_capitalization & c>='A' & c<='Z')*('a'-'A');
			}
			if (buffer[0]==0x05) name[0]=0xe5;
			if (buffer[8]!=' '){
				change_capitalization=(buffer[12] & 0x10)!=0;
				name[i++]='.';
				uint16_t j=8;
				while (j<11){
					const uint8_t c=buffer[j++];
					if (c==' ') break;
					name[i++]=c+(change_capitalization & c>='A' & c<='Z')*('a'-'A');
				}
			}
		}
		{
			name[255]=0;
			uint16_t i=0;
			uint16_t j=0;
			uint8_t c;
			while ((c=name[i++])){
				if (c!='/' & c!='\\' & c!=':' & c!='*' & c!='?' & c>=32 & c<=126) name[j++]=c;
			}
			name[j]=0;
		}
		if (name[0]==0){
			ffo->target.written_entries_count=0;
			return 0;
		}
		ffo->target.written_entries_count+=1;
		ffo->target.attributes=buffer[11];
		ffo->target.file_size=read4(buffer+28);
		ffo->target.entrance_cluster=read2(buffer+26);
		if (!file_system.isFAT16) ffo->target.entrance_cluster |= ((uint32_t)read2(buffer+20) & 0x0fff) << 16;
		ffo->target.walking_cluster=ffo->target.entrance_cluster;
		return 1;
	}
}

struct Directory_Content_Iterator_Arguments {
	struct Folder_File_Object* ffo;
	uint32_t current_cluster;
	uint32_t current_offset;
	uint8_t hide_mask;
	bool had_io_error;
};

// returns 1 if another directory was found. 0 if no more directories exist or there was an io error
bool fat_directory_content_iterator(struct Directory_Content_Iterator_Arguments* dcia){
	dcia->ffo->target.written_entries_count=0;
	dcia->ffo->is_target_root=0;
	dcia->had_io_error=1;
	uint32_t cluster_size=file_system.cluster_size;
	if (dcia->current_cluster==0){
		if (file_system.isFAT16){
			cluster_size=(file_system.cluster_zero_offset - file_system.root_dir_offset)  * (uint32_t)512;
		} else {
			dcia->current_cluster=file_system.root_dir_cluster;
		}
	}
	uint8_t buffer[32];
	while (1){
		if (dcia->current_offset>=cluster_size){
			dcia->current_offset=0;
			if (fat_get_next_cluster(&dcia->current_cluster)) return 0;
			if (dcia->current_cluster==0){
				// no more directory entries to find
				dcia->had_io_error=0;
				memset(&dcia->ffo->target,0,sizeof(struct Target_Folder_File));
				return 0;
			}
		}
		if (cache_read_at_cluster(dcia->current_cluster,dcia->current_offset,32,buffer)) return 0;
		const bool is_finished=fat_interpret_directory_entry(dcia->current_cluster,dcia->current_offset,buffer,dcia->ffo);
		dcia->current_offset+=32;
		if (is_finished){
			// a directory entry listing is finished being read
			if ((dcia->hide_mask & dcia->ffo->target.attributes)==0){
				// then the directory entry listing that was just read is not masked by the hide_mask
				dcia->had_io_error=0;
				return 1;
			}
		}
	}
}

// should only be called when the target is a valid folder
void fat_enter_target_folder(struct Folder_File_Object* ffo){
	ffo->parent_folder_entrance_cluster=ffo->target.entrance_cluster;
	ffo->is_target_root=0;
	memset(&ffo->target,0,sizeof(struct Target_Folder_File));
}

// case insensitive, does not do pattern matching
bool is_filename_match(const uint8_t* source_name, const uint8_t* test_name, uint16_t length){
	for (uint16_t i=0;i<length;i++){
		uint8_t c0=source_name[i];
		c0+=(c0>='A' & c0<='Z')*('a'-'A');
		uint8_t c1=test_name[i];
		c1+=(c1>='A' & c1<='Z')*('a'-'A');
		if (c0==0 | c1==0 | c0!=c1) return 0;
	}
	return test_name[length]==0;
}

// return value string is allocated on heap
uint8_t* normalize_path(const uint8_t* path){
	uint32_t length=strlen((const char*)path);
	_isNextAllocFromKernel=1;
	uint8_t* normalized_path=malloc(length+1);
	uint32_t i;
	for (i=0;i<=length;i++){
		if (path[i]=='\\') normalized_path[i]='/';
		else normalized_path[i]=path[i];
	}
	bool matched_rerwite_rule=1;
	while (matched_rerwite_rule){
		matched_rerwite_rule=0;
		if (normalized_path[0]=='/'){
			matched_rerwite_rule=1;
			length-=1;
			for (i=0;i<=length;i++){
				normalized_path[i]=normalized_path[i+1];
			}
			continue;
		}
		if (normalized_path[0]=='.' && normalized_path[1]=='/'){
			matched_rerwite_rule=1;
			length-=2;
			for (i=0;i<=length;i++){
				normalized_path[i]=normalized_path[i+2];
			}
			continue;
		}
		if (normalized_path[0]=='.' && normalized_path[1]=='.' && normalized_path[2]=='/'){
			matched_rerwite_rule=1;
			length-=3;
			for (i=0;i<=length;i++){
				normalized_path[i]=normalized_path[i+3];
			}
			continue;
		}
		if (length>0 && normalized_path[length-1]=='/'){
			matched_rerwite_rule=1;
			length-=1;
			normalized_path[length]=0;
			continue;
		}
		if (length>1 && (normalized_path[length-2]=='/' & normalized_path[length-1]=='.')){
			matched_rerwite_rule=1;
			length-=2;
			normalized_path[length]=0;
			continue;
		}
		if ((length==1 && (normalized_path[0]=='.')) || (length==2 && (normalized_path[0]=='.' & normalized_path[1]=='.'))){
			length=0;
			normalized_path[0]=0;
			break; // there can't be more rewrite rules
		}
		for (i=1;i<length;){
			if (normalized_path[i -1]=='/' & normalized_path[i -0]=='/'){
				matched_rerwite_rule=1;
				for (;i<=length;i++){
					normalized_path[i -1]=normalized_path[i];
				}
				length-=1;
				i=1;
			} else i++;
		}
		for (i=3;i<=length;){
			if (normalized_path[i -3]=='/' & normalized_path[i -2]=='.' & normalized_path[i -1]=='.' & (normalized_path[i -0]==0 | normalized_path[i -0]=='/')){
				matched_rerwite_rule=1;
				i -=3;
				uint32_t k=0;
				uint32_t j=0;
				for (;j<i;j++){
					if (normalized_path[j]=='/') k=j;
				}
				j=i+3;
				uint32_t s = j -k;
				for (;j<=length;j++){
					normalized_path[j -s]=normalized_path[j];
				}
				length -=s;
				i=3;
			} else i++;
		}
	}
	return normalized_path;
}


/*
return values:
0-no error, path found
1-path not found
2-io error
3-path invalid error (a name length too long)
4-path invalid error (a name length was 0)
5-cannot enter a file because it isn't a folder
6-internal error (due to last path item can't be "." or ".." (it isn't compatible with my method of storing the parent directory in a way that allows the target to be modified))
*/
uint8_t fat_find_normalized_path_target(uint8_t* path, struct Folder_File_Object* ffo, uint8_t hide_mask){
	memset(ffo,0,sizeof(struct Folder_File_Object));
	ffo->target.attributes = FAT_ATTRIB_DIR;
	ffo->is_target_root = 1;
	bool found_submatch;
	uint16_t seperator_index;
	uint8_t* ffo_target_name=ffo->target.name;
	uint8_t* upperpath;
	struct Directory_Content_Iterator_Arguments dcia = {.ffo = ffo, .hide_mask = hide_mask};
	if (path[0]==0) return 0;
	while (1){
		found_submatch=0;
		{
			upperpath = (uint8_t*)strchr((char*)path,'/');
			if (upperpath==NULL) upperpath = strlen((char*)path) + path;
			if ((upperpath - path)>255) return 3;
			seperator_index = upperpath - path;
			if (seperator_index==0) return 4;
		}
		fat_enter_target_folder(ffo);
		dcia.current_cluster=ffo->parent_folder_entrance_cluster;
		dcia.current_offset=0;
		while (fat_directory_content_iterator(&dcia)){
			if (is_filename_match(path,ffo_target_name,seperator_index)){
				found_submatch = 1; // if the "break" occures, then a submatch has actually been found
				path = path + seperator_index;
				if (path[0]=='/') path++;
				if (path[0]==0){
					if (ffo_target_name[0]=='.' & (ffo_target_name[1]==0 | (ffo_target_name[1]=='.' & ffo_target_name[2]==0))){
						return 6;
					}
					return 0;
				}
				if ((ffo->target.attributes & FAT_ATTRIB_DIR)==0) return 5;
				break;
			}
		}
		if (!found_submatch) return 1 + dcia.had_io_error;
	}
}


uint8_t fat_find_path_target(const uint8_t* path, struct Folder_File_Object* ffo, uint8_t hide_mask){
	uint8_t* normalized_path=normalize_path(path);
	const uint8_t res=fat_find_normalized_path_target(normalized_path,ffo,hide_mask);
	free(normalized_path);
	return res;
}


/*
returns 0 on success, 1 if location is outside of file size (and thus was not changed) or the whence was set to an invalid number or a directory was given for ffo
walking_cluster may have been changed if the return value is 0.
if walking_cluster is 0 after this function returns, then fat_true_seek() will have to be used to actually find the walking_cluster for the new walking_position
*/
bool fat_lazy_seek(struct Folder_File_Object* ffo, int32_t offset, uint8_t whence){
	if ((ffo->target.attributes & FAT_ATTRIB_DIR)!=0) return 1;
	const uint8_t offset_sign=((offset > 0)<<1) | ((offset < 0)<<0);
	const uint32_t offset_u=(offset_sign & 1)?(uint32_t)(-offset):(uint32_t)offset;
	switch (whence){
		case SEEK_SET:;
		if ((offset_sign & 2)!=0){
			if (ffo->target.file_size > offset_u){
				return 1;
			} else {
				ffo->target.walking_cluster = 0;
				ffo->target.walking_position = offset_u;
				return 0;
			}
		} else if ((offset_sign & 1)!=0){
			return 1;
		} else {
			// then offset must equal 0
			ffo->target.walking_cluster = ffo->target.entrance_cluster;
			return 0;
		}
		case SEEK_CUR:;
		if ((offset_sign & 2)!=0){
			if ((ffo->target.file_size < ffo->target.walking_position) | (ffo->target.file_size < offset_u) | (ffo->target.file_size < ffo->target.walking_position + offset_u)){
				// ^^^ those checks are for eliminating the possibility of incorrect behaviour if (ffo->target.walking_position + offset_u > maximum_uint32)
				return 1;
			} else {
				ffo->target.walking_cluster = 0;
				ffo->target.walking_position += offset_u;
				return 0;
			}
		} else if ((offset_sign & 1)!=0){
			if ((ffo->target.walking_position > offset_u) | (ffo->target.file_size > ffo->target.walking_position - offset_u)){
				return 1;
			} else {
				ffo->target.walking_cluster = 0;
				ffo->target.walking_position -= offset_u;
				return 0;
			}
		} else {
			// then offset must equal 0
			return (ffo->target.file_size > ffo->target.walking_position);
		}
		case SEEK_END:;
		if ((offset_sign & 2)!=0){
			return 1;
		} else if ((offset_sign & 1)!=0){
			if (offset_u > ffo->target.file_size){
				return 1;
			} else {
				ffo->target.walking_cluster = 0;
				ffo->target.walking_position = ffo->target.file_size - offset_u;
				return 0;
			}
		} else {
			// then offset must equal 0
			ffo->target.walking_cluster = 0;
			ffo->target.walking_position = ffo->target.file_size;
			return 0;
		}
		default:;return 1;
	}
	return 1;
}

/*
fat_true_seek finds the walking_cluster for the current walking_position using the entrance_cluster.

return values:
0-success
1-if the current walking_position is out of bounds of the file_size
2-if there was a file system corruption error (file_size indicated file was larger then what it had sectors for)
3-if an io error occured
4-if it was called on a directory (which is invalid)
*/
uint8_t fat_true_seek(struct Folder_File_Object* ffo){
	if ((ffo->target.attributes & FAT_ATTRIB_DIR)!=0) return 4;
	ffo->target.walking_cluster = 0;
	if (ffo->target.walking_position >= ffo->target.file_size) return 1;
	ffo->target.walking_cluster = ffo->target.entrance_cluster;
	uint32_t temporary_reversed_position = ffo->target.walking_position;
	while (temporary_reversed_position >= file_system.cluster_size){
		temporary_reversed_position-=file_system.cluster_size;
		if (fat_get_next_cluster(&ffo->target.walking_cluster)) return 3;
		if (ffo->target.walking_cluster==0) return 2;
	}
	return 0;
}




/*
buffer_size is a pointer to a value that holds the buffers maxiumum size. after this function returns it will have been set to the number of bytes written to the buffer.

return values:
0-buffer was filled to it's maxiumum, no errors occured
1-buffer was partially filled due to reaching the end of the file
2-buffer was partially filled due to an io error
3-buffer was not filled due to there being nothing to fill (this would happen if the given buffer_size is 0)
4-buffer was not filled due to the position being at the end of the file
5-buffer was not filled due to the position being beyond the end of the file
6-buffer was not filled due to an io error
7-buffer was not filled due to the target being a directory
8-buffer was not filled due to file having no contents (this would happen if it's first data cluster is 0)
9-buffer may have been filled but was stopped due to a file system corruption error (file_size indicated file was larger then what it had sectors for)
*/
uint8_t fat_read_target_file(struct Folder_File_Object* ffo, uint8_t* buffer, uint32_t* buffer_size){
	bool eof_reached=0;
	uint32_t buffer_len=*buffer_size;
	*buffer_size=0;
	if ((ffo->target.attributes & FAT_ATTRIB_DIR)!=0) return 7;
	if (ffo->target.entrance_cluster==0) return 8;
	if (ffo->target.walking_position >= ffo->target.file_size) return 4 + (ffo->target.walking_position > ffo->target.file_size);
	{
		uint32_t file_left = ffo->target.file_size - ffo->target.walking_position;
		if (buffer_len > file_left){
			buffer_len = file_left;
			eof_reached = 1;
		}
	}
	uint32_t buffer_left=buffer_len;
	if (buffer_len==0) return 3;
	if (ffo->target.walking_cluster==0){
		// then the walking_cluster needs to be found based on the walking_position
		uint8_t seek_result=fat_true_seek(ffo);
		if (seek_result!=0){
			if (seek_result==1) return 5;
			if (seek_result==2) return 9;
			if (seek_result==3) return 6;
			if (seek_result==4) return 7;
			
			return 6; // honestly, this error is more like an assertion error though
		}
	}
	uint32_t initial_offset_in_cluster = ffo->target.walking_position % file_system.cluster_size;
	uint32_t rolling_offset_in_cluster = initial_offset_in_cluster;
	bool is_last;
	while (1){
		if (ffo->target.walking_cluster==0){
			*buffer_size = buffer_len - buffer_left;
			return 9;
		}
		uint32_t copy_length = file_system.cluster_size - initial_offset_in_cluster;
		is_last = copy_length >= buffer_left;
		if (is_last) copy_length = buffer_left;
		if (cache_read_at_cluster(ffo->target.walking_cluster,initial_offset_in_cluster,copy_length,buffer)){
			*buffer_size = buffer_len - buffer_left;
			return 2;
		}
		initial_offset_in_cluster = 0;
		ffo->target.walking_position += copy_length;
		buffer_left -= copy_length;
		buffer += copy_length;
		rolling_offset_in_cluster += copy_length;
		if (rolling_offset_in_cluster >= file_system.cluster_size){
			rolling_offset_in_cluster -= file_system.cluster_size;
			if (fat_get_next_cluster(&ffo->target.walking_cluster)){
				ffo->target.walking_cluster = 0;
				*buffer_size = buffer_len - buffer_left;
				return 2;
			}
			// at this point if (ffo->target.walking_cluster==0) it doesn't always matter. If it does matter, then the next time around this while loop an error is given
		}
		if (is_last){
			*buffer_size=buffer_len;
			return eof_reached;
		}
	}
}




/*
the parameter ptr_start_cluster should not be NULL
if *ptr_start_cluster is 0, then a new cluster chain is allocated and *ptr_start_cluster is written with the start of the new chain
if *ptr_start_cluster is not 0, then clusters are appended to that cluster. That cluster should contain a "last cluster" value when this function was called
return values:
0-successful
1-failure due to incorrect parameters
2-failure due to out of space on disk
3-failure due to io error
*/
uint8_t fat_add_clusters(uint32_t* ptr_start_cluster, uint32_t cluster_count){
	const uint32_t start_cluster=*ptr_start_cluster;
	if (cluster_count==0 | start_cluster==1 | start_cluster>=file_system.data_cluster_count) return 1;
	uint32_t effective_start_cluster=start_cluster;
	uint32_t cluster_number=1;
	uint32_t clusters_found=0;
	uint32_t cluster_value;
	if (start_cluster!=0){
		cluster_number=start_cluster;
		if (cache_read_at_table(start_cluster,&cluster_value)) return 3;
		if ((cluster_value & 0x80000000)==0) return 1;
	}
	while (1){
		if (++cluster_number>=file_system.data_cluster_count) cluster_number=2;
		if (cluster_number==start_cluster) return 2;
		if (cache_read_at_table(cluster_number,&cluster_value)) return 3;
		if ((cluster_value & 0x10000000)!=0){
			if (clusters_found==0 & start_cluster==0) effective_start_cluster=cluster_number;
			if (++clusters_found==cluster_count) break;
		}
	}
	uint32_t cluster_previous_hit=cluster_number;
	*ptr_start_cluster=cluster_number;
	if (cache_write_at_table(cluster_number,file_system.isFAT16?(unsigned)FAT16_CLUSTER_LAST_MAX:FAT32_CLUSTER_LAST_MAX)) return 3;
	if (--clusters_found==0) return 0;
	while (1){
		if (--cluster_number<2) cluster_number=file_system.data_cluster_count - 1;
		if (cluster_number==effective_start_cluster){
			if (cache_write_at_table(cluster_number,cluster_previous_hit)) return 3;
			return 0;
		}
		if (cache_read_at_table(cluster_number,&cluster_value)) return 3;
		if ((cluster_value & 0x10000000)!=0){
			if (cache_write_at_table(cluster_number,cluster_previous_hit)) return 3;
			cluster_previous_hit=cluster_number;
		}
	}
}

/*
deallocates a cluster chain starting with cluster_start.
it will set all clusters (except bad_cluster or reserved_cluster) to free in the chain.
if set_start_to_end_cluster==true, then it will set the cluster_start to last_cluster, otherwise it will set cluster_start to cluster_free just like the others in the chain.
However, cluster_start will not be set if cluster_start is bad_cluster or reserved_cluster.
returns:
0-success, the removed chain had ended with a last_cluster entry
1-success(ish), the removed chain had ended with a free_cluster, reserved_cluster, or bad_cluster entry. (this includes if start_cluster held one of those 3 values)
2-failure, io error prevented completion
*/
uint8_t fat_remove_clusters(uint32_t cluster_start,bool set_start_to_end_cluster){
	uint8_t rv=0;
	uint32_t cluster_current;
	uint32_t cluster_next=cluster_start;
	while (1){
		cluster_current=cluster_next;
		if (cache_read_at_table(cluster_current,&cluster_next)) return 2;
		if ((cluster_next & 0xf0000000)!=0){
			rv=(cluster_next & 0x80000000)==0;
			if (!rv){
				if (cache_write_at_table(cluster_current,file_system.isFAT16?(unsigned)FAT16_CLUSTER_FREE:FAT32_CLUSTER_FREE)) return 2;
			}
			if (set_start_to_end_cluster){
				if (cache_read_at_table(cluster_start,&cluster_next)) return 2;
				if ((cluster_next & 0x60000000)==0){
					if (cache_write_at_table(cluster_start,file_system.isFAT16?(unsigned)FAT16_CLUSTER_LAST_MAX:FAT32_CLUSTER_LAST_MAX)) return 2;
				}
			}
			return rv;
		}
		if (cache_write_at_table(cluster_current,file_system.isFAT16?(unsigned)FAT16_CLUSTER_FREE:FAT32_CLUSTER_FREE)) return 2;
	}
}


/*
ffo should not be targeting a "." or ".." entry (or have that name).
The target should not have a name length of 0.
The target should not be the root (obviously, because there is no directory entry for the root directory - it is the root directory).
The name will be modified to contain only valid characters.
Checks for name validity are performed after the name is modified to contain only valid characters.
LFN entries are made even for names which wouldn't need them.

return values:
0-successful
1-ffo is not valid for this function
2-could not write due to no space on disk (this could also occur if there is no space left on the fat16 root directory)
3-io error prevented completion
4-internal error
*/
uint8_t fat_write_directory_entry(struct Folder_File_Object* ffo, bool stop_after_removal){
	uint8_t buffer0[32];
	uint8_t buffer1[32];
	memset(buffer0,0xe5,sizeof(buffer0));
	memset(buffer1,0xff,sizeof(buffer1));
	uint8_t* name=ffo->target.name;
	{
		name[255]=0;
		uint16_t i=0;
		uint16_t j=0;
		uint8_t c;
		while ((c=name[i++])){
			if (c!='/' & c!='\\' & c!=':' & c!='*' & c!='?' & c>=32 & c<=126) name[j++]=c;
		}
		name[j]=0;
	}
	if ((name[0]==0) | (name[0]=='.' & name[1]==0) | (name[0]=='.' & name[1]=='.' & name[2]==0) | ffo->is_target_root) return 1;
	const uint16_t name_len=strlen((const char*)name);
	const uint16_t entry_count_needed=((name_len+12)/13)+1;
	if (name_len>256 | name_len==0 | entry_count_needed<2) return 1; // I don't think this could be possible anyway at this point, but just to make sure
	const uint32_t cluster_size=(ffo->parent_folder_entrance_cluster==0 & file_system.isFAT16)?file_system.fat16_root_max:file_system.cluster_size;
	uint32_t directory_entry_cluster_walk;
	uint32_t directory_entry_cluster_walk_next;
	uint32_t directory_entry_offset_walk;
	if (ffo->target.written_entries_count!=0){
		directory_entry_cluster_walk=ffo->target.directory_entry_cluster;
		directory_entry_offset_walk=ffo->target.directory_entry_offset;
		do {
			if (cache_write_at_cluster(directory_entry_cluster_walk,directory_entry_offset_walk,32,buffer0)) return 3;
			directory_entry_offset_walk+=32;
			if (directory_entry_offset_walk>=cluster_size){
				// kinda weird if this would be needed
				directory_entry_offset_walk=0;
				directory_entry_cluster_walk_next=directory_entry_cluster_walk;
				if (fat_get_next_cluster(&directory_entry_cluster_walk_next)) return 3;
				directory_entry_cluster_walk=directory_entry_cluster_walk_next;
				if (directory_entry_cluster_walk==0){
					// very weird... but continuable
					break;
				}
			}
		} while (--ffo->target.written_entries_count!=0);
		directory_entry_cluster_walk=ffo->target.directory_entry_cluster;
		directory_entry_offset_walk=ffo->target.directory_entry_offset;
	} else {
		directory_entry_cluster_walk=ffo->parent_folder_entrance_cluster;
		directory_entry_offset_walk=0;
	}
	ffo->target.written_entries_count=0;
	if (stop_after_removal){
		return 0;
	}
	{
		uint16_t entry_count_found=0;
		uint8_t first_byte_of_directory_entry;
		bool previous_avalible=0;
		bool current_avalible=0;
		while (1){
			if (cache_read_at_cluster(directory_entry_cluster_walk,directory_entry_offset_walk,1,&first_byte_of_directory_entry)) return 3;
			if (first_byte_of_directory_entry==0){
				first_byte_of_directory_entry=0xe5;
				if (cache_write_at_cluster(directory_entry_cluster_walk,directory_entry_offset_walk,32,buffer0)) return 3;
			}
			previous_avalible=current_avalible;
			current_avalible=first_byte_of_directory_entry==0xe5;
			if (!previous_avalible & current_avalible){
				ffo->target.directory_entry_cluster=directory_entry_cluster_walk;
				ffo->target.directory_entry_offset=directory_entry_offset_walk;
			}
			if (current_avalible){
				if (++entry_count_found==entry_count_needed){
					break;
				}
			}
			directory_entry_offset_walk+=32;
			if (directory_entry_offset_walk>=cluster_size){
				if (file_system.isFAT16 & directory_entry_cluster_walk==0) return 2;
				directory_entry_offset_walk=0;
				current_avalible=0;
				entry_count_found=0;
				while (1){
					directory_entry_cluster_walk_next=directory_entry_cluster_walk;
					if (fat_get_next_cluster(&directory_entry_cluster_walk_next)) return 3;
					if (directory_entry_cluster_walk_next==0){
						directory_entry_cluster_walk_next=directory_entry_cluster_walk;
						const uint8_t res=fat_add_clusters(&directory_entry_cluster_walk_next,1);
						if (res!=0){
							if (res==1) return 2;
							return 3;
						}
					} else {
						directory_entry_cluster_walk=directory_entry_cluster_walk_next;
						const uint8_t z=0;
						do {
							if (cache_write_at_cluster(directory_entry_cluster_walk,directory_entry_offset_walk,1,&z)) return 3;
							directory_entry_offset_walk+=32;
						} while (directory_entry_offset_walk<cluster_size);
						directory_entry_offset_walk=0;
						break;
					}
				}
			}
		}
	}
	{ // 8.3 name/entry generation. I do not verify that there is no duplicate 8.3 names, because what usable system doesn't use LFN anyway?
		uint8_t c;
		uint16_t i=0;
		uint16_t p=0;
		while ((c=name[++i])){
			if (c=='.') p=i;
		}
		if (p>250) p=250;
		buffer0[ 0]=(name[0]==' ' | name[0]=='.')?'_':name[0];
		buffer0[ 6]='~';
		buffer0[ 7]='1';
		buffer0[ 8]=name[p+1];
		buffer0[ 8]=(name_len>(p+1))?((buffer0[ 8]==' ' | buffer0[ 8]=='.')?'_':buffer0[ 8]):' ';
		buffer0[ 9]=name[p+2];
		buffer0[ 9]=(name_len>(p+2))?((buffer0[ 9]==' ' | buffer0[ 9]=='.')?'_':buffer0[ 9]):' ';
		buffer0[10]=name[p+3];
		buffer0[10]=(name_len>(p+3))?((buffer0[10]==' ' | buffer0[10]=='.')?'_':buffer0[10]):' ';
		for (i=1;i<6;i++){
			buffer0[i]=(name_len>i & p>i)?((name[i]==' ' | name[i]=='.')?'_':name[i]):' ';
		}
		for (i=0;i<11;i++){
			if (buffer0[i]>='a' & buffer0[i]<='z') buffer0[i]-='a'-'A';
		}
		buffer0[11]=ffo->target.attributes;
		buffer0[12]=0;
		// time/date values below
		buffer0[13]=0;
		buffer0[14]=0;
		buffer0[15]=0;
		buffer0[16]=80;
		buffer0[17]=0;
		buffer0[18]=80;
		buffer0[19]=1;
		
		buffer0[22]=0;
		buffer0[23]=0;
		buffer0[24]=80;
		buffer0[25]=1;
		// time/date values above
		if (file_system.isFAT16){
			buffer0[20]=0;
			buffer0[21]=0;
		} else {
			buffer0[20]=(ffo->target.entrance_cluster>>16)&0xff;
			buffer0[21]=(ffo->target.entrance_cluster>>24)&0xff;
		}
		buffer0[26]=(ffo->target.entrance_cluster>> 0)&0xff;
		buffer0[27]=(ffo->target.entrance_cluster>> 8)&0xff;
		buffer0[28]=(ffo->target.file_size>> 0)&0xff;
		buffer0[29]=(ffo->target.file_size>> 8)&0xff;
		buffer0[30]=(ffo->target.file_size>>16)&0xff;
		buffer0[31]=(ffo->target.file_size>>24)&0xff;
	}
	buffer1[13]=(ffo->target.checksum=fat_calc_8_3_checksum(buffer0));
	buffer1[11]=0x0f;
	buffer1[12]=0;
	buffer1[26]=0;
	buffer1[27]=0;
	directory_entry_cluster_walk=ffo->target.directory_entry_cluster;
	directory_entry_offset_walk=ffo->target.directory_entry_offset;
	for (uint16_t i=0;i<entry_count_needed -1;i++){
		buffer1[0]=((entry_count_needed -1)-i)&0x1f;
		uint16_t pn=(buffer1[0]-1)*13;
		uint16_t pi;
		const uint8_t char_mapping[13]={ 1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30 };
		if (i==0){
			buffer1[0] |= 0x40;
			for (pi=0;pn<=name_len & pi<13;pn++,pi++){
				buffer1[char_mapping[pi]+0]=name[pn];
				buffer1[char_mapping[pi]+1]=0;
			}
		} else {
			if (i==1){
				for (pi=0;pi<13;pi++){
					buffer1[char_mapping[pi]+1]=0;
				}
			}
			for (pi=0;pi<13;pn++,pi++){
				buffer1[char_mapping[pi]]=name[pn];
			}
		}
		if (cache_write_at_cluster(directory_entry_cluster_walk,directory_entry_offset_walk,32,buffer1)) return 3;
		directory_entry_offset_walk+=32;
		if (directory_entry_offset_walk>=cluster_size) return 4;
	}
	if (cache_write_at_cluster(directory_entry_cluster_walk,directory_entry_offset_walk,32,buffer0)) return 3;
	ffo->target.written_entries_count=entry_count_needed;
	return 0;
}


/*
buffer_size is a pointer to a value that holds the buffers maxiumum size. after this function returns it will have been set to the number of bytes written to the file.

return values:
0-buffer was written to it's maxiumum, no errors occured
1-buffer may have been written due to an io error
2-buffer was not written due to there being nothing to write (this would happen if the given buffer_size is 0)
3-buffer was not written due to the position being beyond the end of the file
4-buffer was not written due to the target not being opened correctly for writing
5-buffer was not written due to the target being a directory
6-buffer was not written due to out of space on disk
7-internal error, buffer may have been written
8-buffer was not written, maxiumum file size requirement would be violated
9-buffer may have been written but was stopped due to a file system corruption error (file_size indicated file was larger then what it had sectors for)
*/
uint8_t fat_write_target_file(struct Folder_File_Object* ffo, const uint8_t* buffer, uint32_t* buffer_size){
	uint32_t buffer_len=*buffer_size;
	uint32_t buffer_left=buffer_len;
	*buffer_size=0;
	if ((ffo->target.attributes & FAT_ATTRIB_DIR)!=0) return 5;
	if (ffo->target.walking_position > ffo->target.file_size) return 3;
	if (buffer_len==0) return 2;
	if ((((buffer_len>>1)+(ffo->target.walking_position>>1)+((buffer_len&1)&(ffo->target.walking_position&1)))&0x80000000)!=0) return 8;
	if (ffo->target.entrance_cluster==0 | !ffo->target.is_open_for_writing) return 4;
	const uint32_t initial_offset = ffo->target.walking_position % file_system.cluster_size;
	if (ffo->target.walking_position!=0 & ffo->target.walking_position==ffo->target.file_size & ffo->target.walking_cluster==0 & initial_offset==0){
		ffo->target.walking_position--;
		uint8_t res=fat_true_seek(ffo);
		ffo->target.walking_position++;
		uint32_t temp2=ffo->target.walking_cluster;
		ffo->target.walking_cluster=0;
		if (res!=0){
			if (res==2) return 9;
			if (res==3) return 1;
			
			return 7;
		}
		uint32_t temp=temp2;
		if (temp==0) return 7;
		res=fat_add_clusters(&temp,1);
		if (res!=0){
			if (res==2) return 9;
			if (res==3) return 1;
			
			return 7;
		}
		temp=temp2;
		if (fat_get_next_cluster(&temp)) return 1;
		if (temp==0) return 7;
		ffo->target.walking_cluster=temp;
	} else if (ffo->target.walking_cluster==0){
		// then the walking_cluster needs to be found based on the walking_position
		uint8_t seek_result=fat_true_seek(ffo);
		if (seek_result!=0){
			if (seek_result==2) return 9;
			if (seek_result==3) return 1;
			
			return 7;
		}
	}
	uint32_t temporary_walking_position = ffo->target.walking_position;
	uint32_t temporary_walking_cluster = ffo->target.walking_cluster;
	uint32_t temporary_last_walking_cluster=temporary_walking_cluster;
	uint32_t additional_clusters_needed=0;
	uint32_t copy_length;
	uint32_t initial_offset_in_cluster = initial_offset;
	uint32_t rolling_offset_in_cluster = initial_offset_in_cluster;
	bool is_last;
	do {
		copy_length = file_system.cluster_size - initial_offset_in_cluster;
		is_last = copy_length >= buffer_left;
		if (is_last) copy_length = buffer_left;
		initial_offset_in_cluster = 0;
		temporary_walking_position += copy_length;
		buffer_left -= copy_length;
		rolling_offset_in_cluster += copy_length;
		if (rolling_offset_in_cluster >= file_system.cluster_size){
			rolling_offset_in_cluster -= file_system.cluster_size;
			if (temporary_walking_cluster!=0){
				if (fat_get_next_cluster(&temporary_walking_cluster)) return 1;
				if (temporary_walking_cluster!=0){
					temporary_last_walking_cluster=temporary_walking_cluster;
				} else {
					additional_clusters_needed++;
				}
			} else {
				additional_clusters_needed++;
			}
		}
	} while (!is_last);
	if (additional_clusters_needed!=0){
		uint8_t res=fat_add_clusters(&temporary_last_walking_cluster,additional_clusters_needed);
		if (res!=0){
			if (res==2) return 9;
			if (res==3) return 1;
			
			return 7;
		}
	}
	// all needed clusters should now be allocated
	buffer_left = buffer_len;
	initial_offset_in_cluster = initial_offset;
	rolling_offset_in_cluster = initial_offset_in_cluster;
	do {
		if (ffo->target.walking_cluster==0){
			*buffer_size = buffer_len - buffer_left;
			return 9;
		}
		copy_length = file_system.cluster_size - initial_offset_in_cluster;
		is_last = copy_length >= buffer_left;
		if (is_last) copy_length = buffer_left;
		if (cache_write_at_cluster(ffo->target.walking_cluster,initial_offset_in_cluster,copy_length,buffer)){
			*buffer_size = buffer_len - buffer_left;
			return 1;
		}
		initial_offset_in_cluster = 0;
		ffo->target.walking_position += copy_length;
		if (ffo->target.walking_position > ffo->target.file_size) ffo->target.file_size = ffo->target.walking_position;
		buffer_left -= copy_length;
		buffer += copy_length;
		rolling_offset_in_cluster += copy_length;
		if (rolling_offset_in_cluster >= file_system.cluster_size){
			rolling_offset_in_cluster -= file_system.cluster_size;
			if (fat_get_next_cluster(&ffo->target.walking_cluster)){
				ffo->target.walking_cluster = 0;
				*buffer_size = buffer_len - buffer_left;
				return 1;
			}
			if (ffo->target.walking_cluster==0){
				*buffer_size = buffer_len - buffer_left;
				return 9;
			}
		}
	} while (!is_last);
	*buffer_size=buffer_len;
	return 0;
}



/*
No concurrent file access checks are performed. Multiple write handles should be avoided elsewhere. Further, whenever there is a write handle, there should be no read handles on the same file.

return values:
0-success
1-ffo is not valid for this function
2-could not write directory entry due to no space on disk (this could also occur if there is no space left on the fat16 root directory)
3-io error prevented completion
4-internal error
*/
uint8_t fat_open_file(struct Folder_File_Object* ffo, bool open_for_writing){
	ffo->target.walking_position=0;
	ffo->target.is_open_for_writing=open_for_writing;
	if (ffo->target.entrance_cluster==0){
		ffo->target.walking_cluster=0;
		uint8_t res=fat_add_clusters(&ffo->target.entrance_cluster,1);
		if (res!=0){
			ffo->target.is_open_for_writing=0;
			ffo->target.entrance_cluster=0;
			return res;
		}
		ffo->target.walking_cluster=ffo->target.entrance_cluster;
		res=fat_write_directory_entry(ffo,0); // needs to write on open because the file had no cluster
		if (res!=0) ffo->target.is_open_for_writing=0;
		return res;
	}
	return 0;
}


/*
return values:
0-success
1-ffo is not valid for this function
2-could not write directory entry due to no space on disk (this could also occur if there is no space left on the fat16 root directory)
3-io error prevented completion
4-internal error
*/
uint8_t fat_close_file(struct Folder_File_Object* ffo){
	if (ffo->target.is_open_for_writing){
		uint8_t res=fat_write_directory_entry(ffo,0); // needs to write on close for if the file was open for writing
		if (res==0){
			ffo->target.is_open_for_writing=0;
			cache_flush_all();
		}
		return res;
	}
	return 0;
}



/*
works for folders and files. destroys the ffo it is given.
No concurrent access checks occur.
If the target is a directory and there are sub files/folders, then the sub files/folders are lost but the clusters they occupy are not. This should be avoided!

return values:
0-successful
1-success(ish), the removed cluster chain had ended with a free_cluster, reserved_cluster, or bad_cluster entry
2-ffo is not valid for this function
3-io error prevented completion
4-internal error
*/
uint8_t fat_delete_object(struct Folder_File_Object* ffo){
	uint8_t res=fat_write_directory_entry(ffo,1);
	if (res!=0){
		if (res==1) return 2;
		if (res==2) return 4;
		return res;
	}
	if (ffo->target.entrance_cluster!=0){
		res=fat_remove_clusters(ffo->target.entrance_cluster,0);
		if (res!=0){
			if (res==1) return 1;
			if (res==2) return 3;
			return 4;
		}
	}
	memset(ffo,0,sizeof(struct Folder_File_Object));
	return 0;
}




/*
when the function starts, ffo should target the parent directory that the object will be placed in.
if successful, ffo will be written with the data for the created object.
the created object is not opened. if it is a file then it should be opened with fat_open_file before reading/writing
if unsuccessful, the value of ffo when the function exists is undefined.

return values:
0-no error
1-io error
2-file_name invalid error (length too long)
3-file_name invalid error (length was 0)
4-ffo target isn't a folder
5-failure due to out of space on disk
6-internal error
7-name had invalid characters, so no operation was performed
8-name already exists in that directory, so no operation was performed
*/
uint8_t fat_create_object(struct Folder_File_Object* ffo, const uint8_t* file_name, uint8_t attributes){
	if ((ffo->target.attributes & FAT_ATTRIB_DIR)==0) return 4;
	if (file_name[0]==0) return 3;
	if (strlen((const char*)file_name)>255) return 2;
	{
		uint16_t i=0;
		uint8_t c;
		while ((c=file_name[i++])){
			if (c=='/' | c=='\\' | c==':' | c=='*' | c=='?' | c<32 | c>126) return 7;
		}
	}
	fat_enter_target_folder(ffo);
	{
		const uint16_t file_name_len=strlen((const char*)file_name);
		struct Directory_Content_Iterator_Arguments dcia={.ffo = ffo, .hide_mask = 0, .current_cluster=ffo->parent_folder_entrance_cluster, .current_offset=0};
		while (fat_directory_content_iterator(&dcia)){
			if (is_filename_match(file_name,ffo->target.name,file_name_len)) return 8;
		}
		if (dcia.had_io_error) return 1;
	}
	memset(&ffo->target,0,sizeof(struct Target_Folder_File));
	memcpy(ffo->target.name,file_name,strlen((const char*)file_name));
	ffo->target.attributes=attributes;
	uint8_t res;
	if ((attributes & FAT_ATTRIB_DIR)!=0){
		res=fat_add_clusters(&ffo->target.entrance_cluster,1);
		if (res!=0){
			if (res==2) return 5;
			if (res==3) return 1;
			return 6;
		}
		const uint32_t entrance_cluster=ffo->target.entrance_cluster;
		{
			const uint32_t parent_entrance_cluster=ffo->parent_folder_entrance_cluster;
			uint8_t buffer[32]={0};
			buffer[ 0]='.';
			buffer[ 1]=' ';
			buffer[ 2]=' ';
			buffer[ 3]=' ';
			buffer[ 4]=' ';
			buffer[ 5]=' ';
			buffer[ 6]=' ';
			buffer[ 7]=' ';
			buffer[ 8]=' ';
			buffer[ 9]=' ';
			buffer[10]=' ';
			buffer[11]=' ';
			buffer[12]=0x10;
			buffer[26]=(entrance_cluster >> 0);
			buffer[27]=(entrance_cluster >> 8);
			if (!file_system.isFAT16){
				buffer[20]=(entrance_cluster >>16);
				buffer[21]=(entrance_cluster >>24);
			}
			if (cache_write_at_cluster(entrance_cluster, 0,32,buffer)) return 1;
			buffer[ 1]='.';
			buffer[26]=(parent_entrance_cluster >> 0);
			buffer[27]=(parent_entrance_cluster >> 8);
			if (!file_system.isFAT16){
				buffer[20]=(parent_entrance_cluster >>16);
				buffer[21]=(parent_entrance_cluster >>24);
			}
			if (cache_write_at_cluster(entrance_cluster,32,32,buffer)) return 1;
		}
		{
			uint32_t directory_entry_offset_walk=64;
			const uint8_t z=0;
			do {
				if (cache_write_at_cluster(entrance_cluster,directory_entry_offset_walk,1,&z)) return 1;
				directory_entry_offset_walk+=32;
			} while (directory_entry_offset_walk<file_system.cluster_size);
		}
	} else {
		
	}
	res=fat_write_directory_entry(ffo,0);
	if (res!=0){
		if (res==2) return 5;
		if (res==3) return 1;
		return 6;
	}
	return 0;
}

/*
Cleans directory entry data inside a target folder.
This function may modify data, but it doesn't modify any valid entries.
Invalid entries that appear before the last valid entry have their first byte set to 0xe5 (which means they have been deleted).
ffo is not modified in any situation.

returns:
0-successful, some data was modified
1-successful, no data was modified
2-target is not a folder, thus no operation was performed
3-an io error occured
4-internal error
*/
uint8_t fat_clean_folder(const struct Folder_File_Object* ffo_const){
	if ((ffo_const->target.attributes & FAT_ATTRIB_DIR)==0) return 3;
	_isNextAllocFromKernel=1;
	struct Folder_File_Object* ffo=malloc(sizeof(struct Folder_File_Object));
	if (ffo==NULL) return 4;
	memcpy(ffo,ffo_const,sizeof(struct Folder_File_Object));
	fat_enter_target_folder(ffo);
	const uint8_t e5=0xe5;
	bool has_modified=0;
	struct Directory_Content_Iterator_Arguments dcia={.ffo = ffo, .hide_mask = 0, .current_cluster=ffo->parent_folder_entrance_cluster, .current_offset=0};
	uint32_t previous_cluster_walk=dcia.current_cluster;
	uint32_t previous_offset_walk=0;
	const uint32_t cluster_size=(dcia.current_cluster==0 & file_system.isFAT16)?((file_system.cluster_zero_offset - file_system.root_dir_offset)  * (uint32_t)512):file_system.cluster_size;
	while (fat_directory_content_iterator(&dcia)){
		while (ffo->target.directory_entry_cluster!=previous_cluster_walk | ffo->target.directory_entry_offset!=previous_offset_walk){
			if (cache_write_at_cluster(previous_cluster_walk,previous_offset_walk,1,&e5)){
				free(ffo);
				return 3;
			}
			has_modified=1;
			previous_offset_walk+=32;
			if (previous_offset_walk>=cluster_size){
				previous_offset_walk-=cluster_size;
				if (fat_get_next_cluster(&previous_cluster_walk)){
					free(ffo);
					return 3;
				}
				if (previous_cluster_walk==0){
					free(ffo);
					return 4; // these clusters should have just been walked by fat_directory_content_iterator, so there should be a next cluster
				}
			}
		}
		for (uint16_t i=0;i<ffo->target.written_entries_count;i++){
			previous_offset_walk+=32;
			if (previous_offset_walk>=cluster_size){
				previous_offset_walk-=cluster_size;
				if (fat_get_next_cluster(&previous_cluster_walk)){
					free(ffo);
					return 3;
				}
				if (previous_cluster_walk==0){
					free(ffo);
					return 4; // these clusters should have just been walked by fat_directory_content_iterator, so there should be a next cluster
				}
			}
		}
	}
	free(ffo);
	if (dcia.had_io_error) return 3;
	return has_modified;
}




void _putchar_screen(char c);
int16_t ex_stdin_block_appropriate();

bool file_buff_flush(FILE* file){
	if (file==NULL) return 1;
	if (file->file_descriptor<0 | file->file_descriptor>=MAX_FILE_DESCRIPTORS | file->curIObuffMode>2 | ((file->curIObuffMode==1 & (file->file_descriptor==1 | file->file_descriptor==2)) | (file->curIObuffMode==2 & file->file_descriptor==0))) return 1; // file invalid
	if (file->buffType!=IOFBF & file->buffType!=IOLBF) return file->buffType!=IONBF; // either the buffer type is invalid or there is no buffer to flush
	if (file->buffPtr==NULL | file->buffLen==0) return 1; // file invalid
	if (file->curIObuffMode==0) return 0; // nothing to flush, buffer doesn't have orientation
	if (file->curIObuffMode==1){
		if (file->file_descriptor==0){
			return 0; // request is ignored. stdin being "flushed" doesn't make sense because those characters would still waiting to be read
		} else {
			// flushing input files is done by forgeting what was read from disk into the file buffer and move the file position by the size of the ungetc buffer and the file buffer.
			struct Folder_File_Object* ffo=all_open_files.file_object_handles+(file->file_descriptor - 3);
			if (file->ungetBuffPos<_MAX_UNGET){
				ffo->target.walking_position -= _MAX_UNGET - file->ungetBuffPos; // I tried to figure out what posix wants the file position to be for ungetc for more then an hour, and I still can't figure it out so I'm just doing this.
				ffo->target.walking_cluster = 0;
			}
			if (file->buffPos < file->buffLen){
				ffo->target.walking_position -= file->buffLen - file->buffPos;
				ffo->target.walking_cluster = 0;
			}
			file->curIObuffMode=0;
			file->ungetBuffPos=_MAX_UNGET;
			return 0;
		}
	} else {
		if (file->file_descriptor==1){
			for (uint32_t i=0;i<file->buffPos;i++){
				_putchar_screen(file->buffPtr[i]);
			}
			file->buffPos=0;
			file->curIObuffMode=0;
			file->ungetBuffPos=_MAX_UNGET;
			return 0;
		} else if (file->file_descriptor==2){
			for (uint32_t i=0;i<file->buffPos;i++){
				_putchar_screen(file->buffPtr[i]);
			}
			file->buffPos=0;
			file->curIObuffMode=0;
			file->ungetBuffPos=_MAX_UNGET;
			return 0;
		} else {
			// flushing output files is done by writing what is left in the buffer to the file. ungetc is ignored and reset because it should not have been used
			file->ungetBuffPos=_MAX_UNGET;
			uint32_t d=file->buffPos;
			if (d!=0){
				uint8_t res=fat_write_target_file(all_open_files.file_object_handles+(file->file_descriptor - 3),file->buffPtr,&file->buffPos);
				file->buffPos=d - file->buffPos;
				if (res==0 & file->buffPos==0){
					file->curIObuffMode=0;
					return 0;
				}
				switch (res){
					case 0:
					break;
					
					case 3:
					file->errFlags |=1; // I guess eof ?
					default:
					file->errFlags |=2;
					break;
				}
				if (file->buffPos!=0){
					// buffer has been only partially written, so because of the way the buffer is stored it's current state is invalid and must be fixed now
					uint8_t*const b=file->buffPtr;
					d -= file->buffPos;
					const uint32_t e=file->buffLen - d;
					for (uint32_t i=0;i<e;i++){
						b[i]=b[i+d];
					}
				}
				return 1; // there was an error
			}
			file->curIObuffMode=0;
			return 0;
		}
	}
}

FILE* fopen(const char* pathname, const char* mode){
	if (pathname==NULL | mode==NULL | !all_open_files.fs_init_performed) return NULL;
	if (all_open_files.working_directory==NULL){
		_isNextAllocFromKernel=1;
		all_open_files.working_directory=malloc(2);
		if (all_open_files.working_directory==NULL) return NULL;
		all_open_files.working_directory[0]='/';
		all_open_files.working_directory[1]=0;
	}
	uint16_t mode_type;
	if      (strcmp("r"  ,mode)==0) mode_type=0;
	else if (strcmp("rb" ,mode)==0) mode_type=0;
	else if (strcmp("w"  ,mode)==0) mode_type=1;
	else if (strcmp("wb" ,mode)==0) mode_type=1;
	else if (strcmp("a"  ,mode)==0) mode_type=2;
	else if (strcmp("ab" ,mode)==0) mode_type=2;
	else if (strcmp("r+" ,mode)==0) mode_type=3;
	else if (strcmp("rb+",mode)==0) mode_type=3;
	else if (strcmp("r+b",mode)==0) mode_type=3;
	else if (strcmp("w+" ,mode)==0) mode_type=4;
	else if (strcmp("wb+",mode)==0) mode_type=4;
	else if (strcmp("w+b",mode)==0) mode_type=4;
	else if (strcmp("a+" ,mode)==0) mode_type=5;
	else if (strcmp("ab+",mode)==0) mode_type=5;
	else if (strcmp("a+b",mode)==0) mode_type=5;
	else {
		return NULL;
	}
	for (uint16_t fd=3;fd<MAX_FILE_DESCRIPTORS;fd++){
		if (all_open_files.file_descriptor_handles[fd].file_descriptor==-1){
			uint8_t* full_pathname;
			{
				const uint32_t l1=strlen(pathname);
				if (pathname[0]!='/' & pathname[0]!='\\'){
					const uint32_t l0=strlen((const char*)all_open_files.working_directory);
					_isNextAllocFromKernel=1;
					full_pathname=malloc(l0+l1+2);
					if (full_pathname==NULL) return NULL;
					memcpy(full_pathname,all_open_files.working_directory,l0);
					memcpy(full_pathname+(l0+1),pathname,l1);
					full_pathname[l0]='/';
					full_pathname[l0+l1+1]=0;
				} else {
					_isNextAllocFromKernel=1;
					full_pathname=malloc(l1+1);
					if (full_pathname==NULL) return NULL;
					memcpy(full_pathname,pathname,l1);
					full_pathname[l1]=0;
				}
			}
			struct Folder_File_Object* ffo=&(all_open_files.file_object_handles[fd -3]);
			switch (fat_find_path_target(full_pathname,ffo,0)){
				case 0:
				if (ffo->is_target_root) break; // I'm not giving the root to the caller
				if ((ffo->target.attributes & FAT_ATTRIB_DIR)!=0){
					break; // for now at least, no directories from fopen()
				}
				File_Ready_For_Open:;
				free(full_pathname);
				switch (fat_open_file(ffo,mode_type!=0)){
					case 0:
					switch (mode_type){
						case 0:
						case 3:
						break;
						case 1:
						case 4:
						ffo->target.file_size=0;
						// currently, the clusters allocated to that file are not cleared. That probably should be performed but it's not technically required (I think...).
						break;
						case 2:
						case 5:
						ffo->target.walking_position=ffo->target.file_size;
						ffo->target.walking_cluster=0;
						break;
						// any other possibilities here are inconceivable
					}
					FILE* ret_file=&(all_open_files.file_descriptor_handles[fd]);
					memset(ret_file,0,sizeof(FILE));
					ret_file->file_descriptor=fd;
					ret_file->buffType=IONBF;
					ret_file->ungetBuffPos=_MAX_UNGET;
					for (uint16_t i=0;i<MAX_AUTO_BUFF;i++){
						if (!all_open_files.file_auto_buff_taken[i]){
							all_open_files.file_auto_buff_taken[i]=1;
							ret_file->buffType=IOFBF;
							ret_file->buffLen=AUTO_BUF_SIZE;
							ret_file->buffPos=AUTO_BUF_SIZE;
							ret_file->buffPtr=all_open_files.file_auto_buff_content[i].buff;
						}
					}
					return ret_file;
					
					default:
					break;
				}
				return NULL;
				
				case 1:
				if (mode_type==1 | mode_type==2 | mode_type==4 | mode_type==5){
					// then it might be that the file doesn't exist and should be created. The parent directory will be accessed instead, then if that succeeds the file will be created.
					uint32_t j;
					while (1){ // clears any trailing slashes
						if (full_pathname[0]=0){
							free(full_pathname);
							return NULL;
						}
						j=strlen((char*)full_pathname)-1;
						uint8_t c=full_pathname[j];
						if (c=='/' | c=='\\') full_pathname[j]=0;
					}
					j=0;
					for (uint32_t i=0;full_pathname[i]!=0;i++){
						uint8_t c=full_pathname[i];
						if (c=='/' | c=='\\') j=i;
					}
					if (j==0 & full_pathname[0]!='/' & full_pathname[0]!='\\') return NULL; // this would be quite weird...
					full_pathname[j]=0; // terminates the string at the last slash which is before the last name
					j++;
					// now, full_pathname[j] is the first character of the name of the new file (with null termination) and full_pathname[0] is the first character of the full path to the parent directory (with null termination)
					switch (fat_find_path_target(full_pathname,ffo,0)){
						case 0:
						switch (fat_create_object(ffo,full_pathname+j,0)){
							case 0:goto File_Ready_For_Open;
							default:
							break;
						}
						default:
						break;
					}
				}
				default:
				break;
			}
			free(full_pathname);
			return NULL;
		}
	}
	return NULL;
}

int16_t fflush(FILE* file){
	if (file==NULL){
		// flush all file descriptors
		bool f=0;
		for (int16_t i=0;i<MAX_FILE_DESCRIPTORS;i++){
			if (all_open_files.file_descriptor_handles[i].file_descriptor==i){
				f |=file_buff_flush(all_open_files.file_descriptor_handles+i);
			}
		}
		if (cache_flush_all()) return -1;
		return f?-1:0;
	} else {
		if (file_buff_flush(file)) return -1;
	}
	if (cache_flush_all()) return -1;
	return 0;
}

int16_t fclose(FILE* file){
	if (file==NULL) return 0;
	if (file->file_descriptor<0) return 0;
	if (file->file_descriptor<3 | file->file_descriptor>=MAX_FILE_DESCRIPTORS) return -1;
	if (fflush(file)) return -1;
	if (file->buffPtr!=NULL){
		for (uint16_t i=0;i<MAX_AUTO_BUFF;i++){
			if ((void*)(file->buffPtr)==(void*)(all_open_files.file_auto_buff_content[i].buff)){
				all_open_files.file_auto_buff_taken[i]=0;
				break;
			}
		}
	}
	if (fat_close_file(&(all_open_files.file_object_handles[file->file_descriptor - 3]))!=0) return -1;
	all_open_files.file_descriptor_handles[file->file_descriptor].file_descriptor=-1;file->file_descriptor=-1;// it's probably the same pointer, but it should both be done to make sure
	return 0;
}

uint32_t fread(void* buffer,uint32_t size,uint32_t count,FILE* file){
	if (file==NULL) return 0;
	if (file->file_descriptor==1 | file->file_descriptor==2 | file->file_descriptor<0 | file->file_descriptor>=MAX_FILE_DESCRIPTORS | file->curIObuffMode>2 | ((file->curIObuffMode==1 & (file->file_descriptor==1 | file->file_descriptor==2)) | (file->curIObuffMode==2 & file->file_descriptor==0)) | buffer==NULL) return 0;
	if (file->curIObuffMode==2){
		// user should not have called this function (io buffer is in output mode), but it will be tolerated in a reasonable way
		if (file_buff_flush(file)){file->errFlags |=2;return 0;}
	}
	if (file->curIObuffMode==0){
		file->buffPos=file->buffLen;
		file->ungetBuffPos=_MAX_UNGET;
	}
	file->curIObuffMode=1;
	uint32_t total_read=0;
	uint32_t total_left=size*count;
	uint8_t* bufferc=buffer;
	if (total_left==0) return 0;
	while (file->ungetBuffPos<_MAX_UNGET){
		*bufferc++=file->ungetBuff[file->ungetBuffPos++];
		++total_read;
		if (--total_left==0) goto Return;
	}
	if (file->buffPtr!=NULL & (file->buffType==IOFBF | file->buffType==IOLBF) & file->buffLen!=0){
		// fill the buffer if it is empty and the size of the read is less then the size of the buffer
		if (file->buffPos == file->buffLen & total_left < file->buffLen){
			fat_read_target_file(&(all_open_files.file_object_handles[file->file_descriptor - 3]),file->buffPtr,&file->buffPos); // for now, error value is ignored for buffer fill. length returned is expected to be correct.
			uint32_t i=file->buffPos;
			const uint32_t d=file->buffLen - i;
			file->buffPos=d;
			uint8_t*const b=file->buffPtr;
			while (i!=0){
				// buffer has been only partially filled, so because of the way the buffer is stored it's current state is invalid and must be fixed now
				--i;
				b[i+d]=b[i];
			}
		}
		// use buffer if it isn't empty
		if (file->buffPos < file->buffLen){
			uint32_t s=file->buffLen - file->buffPos;
			if (s>total_left) s=total_left;
			memcpy(bufferc,file->buffPtr+file->buffPos,s);
			file->buffPos+=s;
			bufferc+=s;
			total_read+=s;
			total_left-=s;
			if (total_left==0) goto Return;
		}
		assert(file->buffPos == file->buffLen);
	}
	if (file->file_descriptor==0){
		// KEYBOARD - stdin
		while (total_left!=0){
			total_left--;
			total_read++;
			*bufferc++=ex_stdin_block_appropriate();
		}
		goto Return;
	} else {
		uint8_t res;
		{
			uint32_t total_orig=total_left;
			res=fat_read_target_file(&(all_open_files.file_object_handles[file->file_descriptor - 3]),bufferc,&total_left);
			total_read+=total_left;
			total_left=total_orig-total_left;
		}
		switch (res){
			case 0:
			break;
			
			case 1:
			case 4:
			file->errFlags |=1;
			break;
			
			case 5:
			file->errFlags |=3;
			break;
			
			default:
			file->errFlags |=2;
			break;
		}
		goto Return;
	}
	Return:;
	return size<=1?total_read:total_read/size;
}

uint32_t fwrite(const void* buffer,uint32_t size,uint32_t count,FILE* file){
	if (file==NULL) return 0;
	if (file->file_descriptor<=0 | file->file_descriptor>=MAX_FILE_DESCRIPTORS | file->curIObuffMode>2 | ((file->curIObuffMode==1 & (file->file_descriptor==1 | file->file_descriptor==2)) | (file->curIObuffMode==2 & file->file_descriptor==0)) | buffer==NULL) return 0;
	if (file->curIObuffMode==1){
		// user should not have called this function (io buffer is in input mode), but it will be tolerated in a reasonable way.
		// I'm ignoring the ungetc buffer though, I'm not helping them if they are that stupid. And I am kinda starting to hate ungetc.
		if (file_buff_flush(file)) return 0;
	}
	if (file->curIObuffMode==0){
		file->buffPos=0;
		file->ungetBuffPos=_MAX_UNGET;
	}
	file->curIObuffMode=2;
	// ungetc buffer is ignored because if this function is called it should already be empty
	uint32_t total_wrote=0;
	uint32_t total_left=size*count;
	const uint8_t* bufferc=buffer;
	if (total_left==0) return 0;
	if (file->buffPtr!=NULL & (file->buffType==IOFBF | file->buffType==IOLBF) & file->buffLen!=0){
		// if buffer is full, flush the buffer.
		if (file->buffPos == file->buffLen){
			if (file_buff_flush(file)) goto Return;
			file->curIObuffMode=2;
			file->buffPos=0;
		}
		if (total_left > file->buffLen - file->buffPos){
			// in this case, buffer couldn't hold the amount of data that is needed to be written, so just flush the buffer and write it directly.
			if (file_buff_flush(file)) goto Return;
			file->curIObuffMode=2;
			file->buffPos=0;
		} else {
			// in this case, the buffer can probably hold everything that needs to be written.
			if (file->buffPos < file->buffLen){
				// filling up the buffer because it has room
				uint32_t s=file->buffLen - file->buffPos;
				if (s>total_left) s=total_left;
				memcpy(file->buffPtr+file->buffPos,bufferc,s);
				file->buffPos+=s;
				bufferc+=s;
				total_wrote+=s;
				total_left-=s;
				if (file->buffType==IOLBF){
					if (memchr(bufferc,'\n',s)!=NULL){
						if (file_buff_flush(file)) goto Return;
					}
				}
				if (total_left==0) goto Return;
			}
			// if this point is reached, then the buffer actually didn't have enough room for the data. So flush the buffer and finish writing directly.
			if (file_buff_flush(file)) goto Return;
			file->curIObuffMode=2;
			file->buffPos=0;
		}
	}
	if (file->file_descriptor==1){
		// STDOUT
		while (total_left!=0){
			total_left--;
			total_wrote++;
			_putchar_screen(*bufferc++);
		}
	} else if (file->file_descriptor==2){
		// STDERR
		while (total_left!=0){
			total_left--;
			total_wrote++;
			_putchar_screen(*bufferc++);
		}
	} else {
		uint8_t res;
		{
			// direct write
			uint32_t total_orig=total_left;
			res=fat_write_target_file(&(all_open_files.file_object_handles[file->file_descriptor - 3]),bufferc,&total_left);
			total_wrote+=total_left;
			total_left=total_orig-total_left;
		}
		switch (res){
			case 0:
			break;
			
			case 3:
			file->errFlags |=1; // I guess eof ?
			default:
			file->errFlags |=2;
			break;
		}
	}
	Return:;
	return size<=1u?total_wrote:total_wrote/size;
}

int16_t fgetc(FILE* file){
	if (file==NULL) return -1;
	uint8_t c=0;
	if (fread(&c,1,1,file)==1) return c;
	return -1;
}

int16_t fputc(int16_t ch,FILE* file){
	if (file==NULL) return -1;
	const uint8_t c=(uint8_t)ch;
	if (fwrite(&c,1,1,file)==1) return (uint8_t)ch;
	return -1;
}

int16_t fseek(FILE* file,int32_t offset,int16_t whence){
	if (file==NULL) return -1;
	if (file->file_descriptor>=MAX_FILE_DESCRIPTORS) return -1;
	if (file->file_descriptor<3) return -1; // no fseek on stdio,stdout,stderr
	if (file_buff_flush(file)) return -1;
	if (fat_lazy_seek(&(all_open_files.file_object_handles[file->file_descriptor - 3]),offset,whence)) return -1;
	file->errFlags=file->errFlags ^ (file->errFlags & 1);
	return 0;
}

int32_t ftell(FILE* file){
	if (file==NULL) return -1;
	if (file->file_descriptor>=MAX_FILE_DESCRIPTORS) return -1;
	if (file->file_descriptor<3) return -1; // no ftell on stdio,stdout,stderr
	uint32_t r=all_open_files.file_object_handles[file->file_descriptor - 3].target.walking_position;
	if (file->curIObuffMode==1 & file->ungetBuffPos<_MAX_UNGET){
		r -= _MAX_UNGET - file->ungetBuffPos; // I tried to figure out what posix wants the file position to be for ungetc for more then an hour, and I still can't figure it out so I'm just doing this.
	}
	if (file->buffPtr!=NULL & file->buffLen!=0 & (file->buffType==IOFBF | file->buffType==IOLBF)){
		if (file->curIObuffMode==2){
			r += file->buffPos;
		} else if (file->curIObuffMode==1 & file->buffPos < file->buffLen){
			r -= file->buffLen - file->buffPos;
		}
	}
	if (r & 0x80000000) return -1; // can't represent file size with int32_t
	return (int32_t)r;
}

int16_t fgetpos(FILE* file,fpos_t* pos){
	if (file==NULL | pos==NULL) return -1;
	if (file->file_descriptor>=MAX_FILE_DESCRIPTORS) return -1;
	if (file->file_descriptor<3) return -1; // no fgetpos on stdio,stdout,stderr
	uint32_t r=all_open_files.file_object_handles[file->file_descriptor - 3].target.walking_position;
	if (file->curIObuffMode==1 & file->ungetBuffPos<_MAX_UNGET){
		r -= _MAX_UNGET - file->ungetBuffPos; // I tried to figure out what posix wants the file position to be for ungetc for more then an hour, and I still can't figure it out so I'm just doing this.
	}
	if (file->buffPtr!=NULL & file->buffLen!=0 & (file->buffType==IOFBF | file->buffType==IOLBF)){
		if (file->curIObuffMode==2){
			r += file->buffPos;
		} else if (file->curIObuffMode==1 & file->buffPos < file->buffLen){
			r -= file->buffLen - file->buffPos;
		}
	}
	pos->position=r;
	return 0;
}

int16_t fsetpos(FILE* file,fpos_t* pos){
	if (file==NULL | pos==NULL) return -1;
	if (file->file_descriptor>=MAX_FILE_DESCRIPTORS) return -1;
	if (file->file_descriptor<3) return -1; // no fsetpos on stdio,stdout,stderr
	if (file_buff_flush(file)) return -1;
	struct Folder_File_Object* ffo=&(all_open_files.file_object_handles[file->file_descriptor - 3]);
	if (ffo->target.file_size < pos->position) return -1;
	if (ffo->target.walking_position==pos->position) return 0; // if it's the same position, then there is no need to set the position
	ffo->target.walking_position=pos->position;
	ffo->target.walking_cluster=0;
	return 0;
}

int16_t ungetc(int16_t ch, FILE* file){
	if (file==NULL | ch==-1) return -1;
	if (file->file_descriptor<0 | file->file_descriptor>=MAX_FILE_DESCRIPTORS | file->ungetBuffPos==0) return -1;
	switch (file->curIObuffMode){
		case 0:;file->curIObuffMode=1;file->buffPos=file->buffLen;
		case 1:;break;
		default:;return -1;
	}
	return (file->ungetBuff[--file->ungetBuffPos]=(uint8_t)ch); // assignment is intended
}

int16_t rewind(FILE* file){
	if (file==NULL) return -1;
	file->errFlags=0;
	return fseek(file,0,SEEK_SET);
}

void clearerr(FILE* file){
	if (file!=NULL) file->errFlags=0;
}

int16_t feof(FILE* file){
	if (file==NULL) return -1;
	if (file->errFlags & 1) return -1;
	return 0;
}

int16_t ferror(FILE* file){
	if (file==NULL) return -1;
	if (file->errFlags & 2) return -1;
	return 0;
}

/*
todo:

setbuf()
setvbuf()
rename()
remove()

+ a few other small ones

*/


// only open_file_system() should use open_file_system_no_reset_error()
bool open_file_system_no_reset_error(uint8_t partition_index){
	memset(&file_system,0,sizeof(struct File_System));
	partition_index &= 3; // force range to be correct
	uint8_t buffer[37];
	if (cache_read_via_sector(0,450 + partition_index * 16,12,buffer)) return 1;
	if (buffer[0]==0) return 1; // no partition at that index
	const uint32_t partition_offset=read4(buffer+4);
	const uint32_t partition_length=read4(buffer+8);
	
	if (cache_read_via_sector(partition_offset,11,37,buffer)) return 1;
	{
		const uint16_t bytes_per_sector=read2(buffer);
		if (bytes_per_sector!=512) return 1; // this implementation only supports sector sizes of 512 bytes
	}
	file_system.sectors_per_cluster=*(buffer+2);
	if (file_system.sectors_per_cluster==0) return 1;
	const uint16_t reserved_sectors=read2(buffer+3);
	const uint8_t fat_copies=*(buffer+5);
	const uint16_t max_root_entries=read2(buffer+6);
	const uint16_t sector_count_16=read2(buffer+8);
	const uint16_t sectors_per_fat16=read2(buffer+11);
	const uint32_t sector_count_32=read4(buffer+21);
	const uint32_t sectors_per_fat32=read4(buffer+25);
	file_system.root_dir_cluster=read4(buffer+33);
	if (
		(sector_count_16==0 & sector_count_32==0) | // bad volume size
		(sectors_per_fat16!=0 & sectors_per_fat32==0) // not fat16 or fat32
		) return 1;
	const uint32_t sector_count_final=(sector_count_32==0)?sector_count_16:sector_count_32;
	const uint32_t sectors_per_fat_final=(sectors_per_fat16!=0)?sectors_per_fat16:sectors_per_fat32;
	const uint16_t max_root_entry_sectors=((uint32_t)max_root_entries * 32 + 511) >> 9;
	const uint32_t data_sector_count = sector_count_final - reserved_sectors - sectors_per_fat_final * fat_copies - max_root_entry_sectors;
	file_system.data_cluster_count = data_sector_count / file_system.sectors_per_cluster;
	file_system.fat16_root_max=(uint32_t)max_root_entry_sectors * 512;
	file_system.fat_offset=partition_offset + reserved_sectors;
	file_system.cluster_size=(uint32_t)file_system.sectors_per_cluster * 512;
	if (file_system.data_cluster_count < 4085LU){
		return 1; // fat12 not supported
	} else if (file_system.data_cluster_count < 65525LU){
		file_system.isFAT16=1;
		file_system.root_dir_offset=file_system.fat_offset + fat_copies * (uint32_t)sectors_per_fat16;
		file_system.cluster_zero_offset=file_system.root_dir_offset + max_root_entry_sectors;
		file_system.root_dir_cluster=0; // for fat16, root_dir_cluster is forced to be 0
	} else {
		file_system.isFAT16=0;
		file_system.root_dir_offset=0;
		file_system.cluster_zero_offset=file_system.fat_offset + fat_copies * sectors_per_fat_final;
		if (file_system.root_dir_cluster==0) return 1; // for fat32, root_dir_cluster should not be 0
	}
	return 0;
}

// returns 1 if the partition could not be opened, otherwise 0
bool open_file_system(uint8_t partition_index){
	if (open_file_system_no_reset_error(partition_index)){
		memset(&file_system,0,sizeof(struct File_System));
		return 1;
	}
	return 0;
}

// returns 1 on failure, 0 on success. will overwrite the currently mounted file system, and all open files will be forced closed without flushing buffers
bool perform_file_system_init(){
	free(all_open_files.working_directory);
	memset(&all_open_files,0,sizeof(struct All_Open_Files));
	for (uint16_t i=3;i<MAX_FILE_DESCRIPTORS;i++){
		all_open_files.file_descriptor_handles[i].file_descriptor=-1;
	}
	all_open_files.file_descriptor_handles[0].file_descriptor=0;
	all_open_files.file_descriptor_handles[0].curIObuffMode=IONBF;
	all_open_files.file_descriptor_handles[0].buffType=1;
	all_open_files.file_descriptor_handles[0].ungetBuffPos=_MAX_UNGET;
	all_open_files.file_descriptor_handles[1].file_descriptor=1;
	all_open_files.file_descriptor_handles[1].curIObuffMode=IONBF;
	all_open_files.file_descriptor_handles[1].buffType=2;
	all_open_files.file_descriptor_handles[1].ungetBuffPos=_MAX_UNGET;
	all_open_files.file_descriptor_handles[2].file_descriptor=2;
	all_open_files.file_descriptor_handles[2].curIObuffMode=IONBF;
	all_open_files.file_descriptor_handles[2].buffType=2;
	all_open_files.file_descriptor_handles[2].ungetBuffPos=_MAX_UNGET;
	for (uint8_t partition_index=0;partition_index<4;partition_index++){
		if (!open_file_system(partition_index)){
			all_open_files.fs_init_performed=1;
			return 0;
		}
	}
	return 1;
}

struct _print_target{
	_Bool isStr;
	_Bool strMaxHit;
	char* str;
	FILE* stream;
	unsigned long strMax;
	unsigned long writeCount;
};

struct {
	uint8_t ansiBuffer[8];
	bool isAnsiEscapeOccuring;
	uint8_t current_foreground;
	uint8_t current_background;
	uint16_t cursor;
} _terminalCharacterState={
	.current_foreground=255,
	.current_background=0
};

void _print_target_char(struct _print_target* print_target,char c){
	if (print_target->isStr){
		if (!print_target->strMaxHit){
			print_target->str[print_target->writeCount++]=c;
			print_target->strMaxHit=print_target->writeCount>print_target->strMax;
		} else {
			print_target->writeCount++;
		}
	} else {
		fputc(c,print_target->stream);
	}
}

void _putstr(struct _print_target* print_target,const char* str){
	if (str==NULL) str="(null)";
	while (*str!=0){
		_print_target_char(print_target,*(str++));
	}
}

void _put_udeci(struct _print_target* print_target,unsigned long uv){
	bool t=false;
	const uint8_t iStart=(uv&0xFFFF0000)!=0?9:4;
	uint8_t i=iStart;
	uint8_t o[10];
	if (iStart==9){
		do {
			o[i]=uv%10;
			uv=uv/10;
		} while (i--!=0);
	} else {
		uint16_t uvs=uv;
		do {
			o[i]=uvs%10;
			uvs=uvs/10;
		} while (i--!=0);
	}
	i=iStart;
	do {
		char c=o[iStart-i];
		t|=c!=0;
		if (t) _print_target_char(print_target,c+'0');
	} while (i--!=0);
	if (!t) _print_target_char(print_target,'0');
}

void _put_sdeci(struct _print_target* print_target,long sv){
	unsigned long uv=sv;
	bool s = (sv&0x80000000)!=0;
	unsigned long uvc=(uv^(s*0xFFFFFFFF))+s;
	if (s) _print_target_char(print_target,'-');
	_put_udeci(print_target,uvc);
}

void _putinthex(struct _print_target* print_target,unsigned int v){
	for (unsigned int i=0;i<2u;i++){
		unsigned int byte0=((char*)&v)[1u-i];
		unsigned int digit0=(byte0>>4)&0xFu;
		unsigned int digit1=(byte0>>0)&0xFu;
		digit0=digit0+(digit0>=10u)*7u+48u;
		digit1=digit1+(digit1>=10u)*7u+48u;
		_print_target_char(print_target,digit0);
		_print_target_char(print_target,digit1);
	}
}


void _print(struct _print_target* print_target,const char* format,va_list args){
	while (1){
		char c;
		bool formatFinished;
		bool formatTerminateByAnother;
		bool formatTerminateByNull;
		bool formatTerminateByExcess;
		uint16_t formatState0=0;
		uint16_t formatState1;
		uint32_t val;
		switch ((c=*(format++))){
			case 0:
			return;
			case '%':
			FormatStart:;
			formatFinished=false;
			formatState0=1;
			formatState1=0;
			formatTerminateByAnother=0;
			formatTerminateByNull=0;
			formatTerminateByExcess=0;
			do {
				switch ((c=*(format++))){
					case 0:
					formatFinished=1;
					formatTerminateByNull=1;
					break;
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
					break;
					case 'c':
					formatState0=10;
					formatFinished=1;
					break;
					case 'l':
					formatState0+=2;
					break;
					case 'd':
					formatFinished=1;
					break;
					case 'u':
					formatState0+=1;
					formatFinished=1;
					break;
					case 'x':
					case 'X':
					formatState0=7;
					break;
					case '%':
					formatFinished=1;
					if (formatState0==1){
						formatState0=8;
					} else {
						formatTerminateByAnother=1;
					}
					break;
					case 's':
					formatFinished=1;
					if (formatState0==1){
						formatState0=9;
					}
					break;
					default:
					formatFinished=1;
					formatTerminateByExcess=1;
					break;
				}
			} while (!formatFinished);
			switch (formatState0){
				case 1:
				case 2:
				case 7:
				case 10:
				val=va_arg(args,unsigned int);
				break;
				case 3:
				case 4:
				case 9:
				val=va_arg(args,unsigned long);
				break;
			}
			switch (formatState0){
				case 1:
				val=(long)((int)val); // sign extend
				case 3:
				_put_sdeci(print_target,val);
				break;
				case 2:
				case 4:
				_put_udeci(print_target,val);
				break;
				case 7:
				_putinthex(print_target,val);
				break;
				case 8:
				_print_target_char(print_target,'%');
				break;
				case 9:
				_putstr(print_target,(char*)val);
				break;
				case 10:
				_print_target_char(print_target,val&255);
				break;
			}
			if (formatTerminateByNull) {
				return;
			}
			if (formatTerminateByAnother) goto FormatStart;
			if (formatTerminateByExcess) _print_target_char(print_target,c);
			
			break;
			default:
			_print_target_char(print_target,c);
			break;
		}
	}
}


int vsprintf(char *dest, const char *format, va_list args){
	struct _print_target print_target={0};
	print_target.isStr=1;
	print_target.str=dest;
	print_target.strMax=0xFFFFFFFF;
	
	_print(&print_target,format,args);
	
	_print_target_char(&print_target,0);
	return print_target.writeCount;
}

int fprintf(FILE* stream, const char* format, ...){
	struct _print_target print_target={0};
	print_target.stream=stream;
	
	va_list args;
	va_start(args,format);
	_print(&print_target,format,args);
	va_end(args);
	return print_target.writeCount;
}

int printf(const char* format, ...){
	struct _print_target print_target={0};
	print_target.stream=stdout;
	
	va_list args;
	va_start(args,format);
	_print(&print_target,format,args);
	va_end(args);
	return print_target.writeCount;
}

int snprintf(char* dest, unsigned long size, const char* format, ...){
	struct _print_target print_target={0};
	print_target.isStr=1;
	print_target.str=dest;
	print_target.strMax=size;
	
	va_list args;
	va_start(args,format);
	_print(&print_target,format,args);
	va_end(args);
	_print_target_char(&print_target,0);
	return print_target.writeCount;
}




void ps2_command(uint8_t id){
	/* id :
	0 = self test passed message/responce
	1 = self test failed message/responce
	2 = ack responce
	3 = resend responce
	4 = echo responce
	*/
	static uint8_t controller_state=0;
	*((volatile uint8_t*)(0x80000000lu|0x01lu))=1;
	switch (id){
		case 0:{
			if (controller_state!=0){
				controller_state=5;
				return;
			}
			if (*((volatile uint8_t*)(0x80000000lu|0x1800000lu|0x02lu))!=0){
			} else {
				*((volatile uint8_t*)(0x80000000lu|0x1800000lu|0x01lu))=0xED;
				controller_state=1;
			}
		}return;
		case 1:{
		}return;
		case 3:{
		}return;
		case 4:{
		}return;
	}
	switch (controller_state){
		case 0:{
			controller_state=5;
			return;
		}break;
		case 1:{
			*((volatile uint8_t*)(0x80000000lu|0x1800000lu|0x01lu))=0x00;
		}break;
		case 2:{
			*((volatile uint8_t*)(0x80000000lu|0x1800000lu|0x01lu))=0xF3;
		}break;
		case 3:{
			*((volatile uint8_t*)(0x80000000lu|0x1800000lu|0x01lu))=0x60;
		}break;
		case 4:{
			return;
		}break;
		case 5:{
			return;
		}break;
	}
	controller_state++;
}

int16_t ps2_read_byte(){
	/* mod_held :
	bit 0:left shift pressed
	bit 1:right shift pressed
	*/
	static uint8_t mod_held;
	/* in_flags :
	bit 0:break
	bit 1:special 0
	bit 2:special 1
	*/
	static uint8_t in_flags;
	uint8_t c0,c1;
	if (*((volatile uint8_t*)(0x80000000lu|0x1800000lu|0x02lu))!=0){
		c0=*((volatile uint8_t*)(0x80000000lu|0x1800000lu|0x00lu));
		*((volatile uint8_t*)(0x80000000lu|0x1800000lu|0x00lu))=0;
		uint8_t k0,k1,k2,k3,kV;
		k0=*((volatile uint8_t*)(0x80000000lu|0x1800000lu|0x06lu));
		k1=*((volatile uint8_t*)(0x80000000lu|0x1800000lu|0x07lu));
		k2=*((volatile uint8_t*)(0x80000000lu|0x1800000lu|0x04lu));
		k3=*((volatile uint8_t*)(0x80000000lu|0x1800000lu|0x03lu));
		if (k0) *((volatile uint8_t*)(0x80000000lu|0x1800000lu|0x06lu))=0;
		if (k1) *((volatile uint8_t*)(0x80000000lu|0x1800000lu|0x07lu))=0;
		
		kV=((bool)k0<<0) | ((bool)k1<<1) | ((bool)k2<<2) | ((bool)k3<<3);
		// if kV is not 0 then the byte that we have might be incorrect. I'm going to ignore it for now though...
	} else {
		return -1;
	}
	/* byte_to_action0 and byte_to_action1 :
	==0:unexpected byte
	==1:flag "break"
	==2:flag "special 0"
	==3:flag "special 1"
	==4:ack command responce
	==5:resend command responce
	==6:echo command responce
	==7:self test past responce
	>=8:probably finished
	==255:ignore
	*/
	static const uint8_t byte_to_action0[128]={ // shift disabled
		[0x00]=0,
		[0x01]=255,
		[0x02]=0,
		[0x03]=255,
		[0x04]=255,
		[0x05]=255,
		[0x06]=255,
		[0x07]=255,
		[0x08]=0,
		[0x09]=255,
		[0x0a]=255,
		[0x0b]=255,
		[0x0c]=255,
		[0x0d]=9,//tab
		[0x0e]=96,//`
		[0x0f]=0,
		[0x10]=0,
		[0x11]=255,
		[0x12]=14,//left shift
		[0x13]=0,
		[0x14]=255,
		[0x15]=113,//q
		[0x16]=49,//1
		[0x17]=0,
		[0x18]=0,
		[0x19]=0,
		[0x1a]=122,//z
		[0x1b]=115,//s
		[0x1c]=97,//a
		[0x1d]=119,//w
		[0x1e]=50,//2
		[0x1f]=255,
		[0x20]=0,
		[0x21]=99,//c
		[0x22]=120,//x
		[0x23]=100,//d
		[0x24]=101,//e
		[0x25]=52,//4
		[0x26]=51,//3
		[0x27]=255,
		[0x28]=0,
		[0x29]=32,//space
		[0x2a]=118,//v
		[0x2b]=102,//f
		[0x2c]=116,//t
		[0x2d]=114,//r
		[0x2e]=53,//5
		[0x2f]=255,
		[0x30]=0,
		[0x31]=110,//n
		[0x32]=98,//b
		[0x33]=104,//h
		[0x34]=103,//g
		[0x35]=121,//y
		[0x36]=54,//6
		[0x37]=0,
		[0x38]=0,
		[0x39]=0,
		[0x3a]=109,//m
		[0x3b]=106,//j
		[0x3c]=117,//u
		[0x3d]=55,//7
		[0x3e]=56,//8
		[0x3f]=0,
		[0x40]=0,
		[0x41]=44,//,
		[0x42]=107,//k
		[0x43]=105,//i
		[0x44]=111,//o
		[0x45]=48,//0
		[0x46]=57,//9
		[0x47]=0,
		[0x48]=0,
		[0x49]=46,//.
		[0x4a]=47,// /
		[0x4b]=108,//l
		[0x4c]=59,//;
		[0x4d]=112,//p
		[0x4e]=45,//-
		[0x4f]=0,
		[0x50]=0,
		[0x51]=0,
		[0x52]=39,//'
		[0x53]=0,
		[0x54]=91,//[
		[0x55]=61,//=
		[0x56]=0,
		[0x57]=0,
		[0x58]=255,
		[0x59]=15,//right shift
		[0x5a]=10,//enter
		[0x5b]=93,//]
		[0x5c]=0,
		[0x5d]=92,// \   (...)
		[0x5e]=0,
		[0x5f]=0,
		[0x60]=0,
		[0x61]=0,
		[0x62]=0,
		[0x63]=0,
		[0x64]=0,
		[0x65]=0,
		[0x66]=8,//backspace
		[0x67]=0,
		[0x68]=0,
		[0x69]=255,
		[0x6a]=0,
		[0x6b]=18,//left arrow
		[0x6c]=255,
		[0x6d]=0,
		[0x6e]=0,
		[0x6f]=0,
		[0x70]=255,
		[0x71]=127,//del
		[0x72]=19,//down arrow
		[0x73]=255,
		[0x74]=20,//right arrow
		[0x75]=17,//up arrow
		[0x76]=27,//esc
		[0x77]=255,
		[0x78]=255,
		[0x79]=43,//+ kp
		[0x7a]=255,
		[0x7b]=45,//- kp
		[0x7c]=42,//* kp
		[0x7d]=255,
		[0x7e]=255,
		[0x7f]=0
	};
	static const uint8_t byte_to_action1[128]={ // shift enabled
		[0x00]=0,
		[0x01]=255,
		[0x02]=0,
		[0x03]=255,
		[0x04]=255,
		[0x05]=255,
		[0x06]=255,
		[0x07]=255,
		[0x08]=0,
		[0x09]=255,
		[0x0a]=255,
		[0x0b]=255,
		[0x0c]=255,
		[0x0d]=9,//tab
		[0x0e]=126,//~
		[0x0f]=0,
		[0x10]=0,
		[0x11]=255,
		[0x12]=14,//left shift
		[0x13]=0,
		[0x14]=255,
		[0x15]=81,//Q
		[0x16]=33,//!
		[0x17]=0,
		[0x18]=0,
		[0x19]=0,
		[0x1a]=90,//Z
		[0x1b]=83,//S
		[0x1c]=65,//A
		[0x1d]=87,//W
		[0x1e]=64,//@
		[0x1f]=255,
		[0x20]=0,
		[0x21]=67,//C
		[0x22]=88,//X
		[0x23]=68,//D
		[0x24]=69,//E
		[0x25]=36,//$
		[0x26]=35,//#
		[0x27]=255,
		[0x28]=0,
		[0x29]=32,//space
		[0x2a]=86,//V
		[0x2b]=70,//F
		[0x2c]=84,//T
		[0x2d]=82,//R
		[0x2e]=37,//%
		[0x2f]=255,
		[0x30]=0,
		[0x31]=78,//N
		[0x32]=66,//B
		[0x33]=72,//H
		[0x34]=71,//G
		[0x35]=89,//Y
		[0x36]=94,//^
		[0x37]=0,
		[0x38]=0,
		[0x39]=0,
		[0x3a]=77,//M
		[0x3b]=74,//J
		[0x3c]=85,//U
		[0x3d]=38,//&
		[0x3e]=42,//*
		[0x3f]=0,
		[0x40]=0,
		[0x41]=60,//<
		[0x42]=75,//K
		[0x43]=73,//I
		[0x44]=79,//O
		[0x45]=41,//)
		[0x46]=40,//(
		[0x47]=0,
		[0x48]=0,
		[0x49]=62,//>
		[0x4a]=63,//?
		[0x4b]=76,//L
		[0x4c]=58,//:
		[0x4d]=80,//P
		[0x4e]=95,//_
		[0x4f]=0,
		[0x50]=0,
		[0x51]=0,
		[0x52]=34,//"
		[0x53]=0,
		[0x54]=123,//{
		[0x55]=43,//+
		[0x56]=0,
		[0x57]=0,
		[0x58]=255,
		[0x59]=15,//right shift
		[0x5a]=10,//enter
		[0x5b]=125,//}
		[0x5c]=0,
		[0x5d]=124,// |
		[0x5e]=0,
		[0x5f]=0,
		[0x60]=0,
		[0x61]=0,
		[0x62]=0,
		[0x63]=0,
		[0x64]=0,
		[0x65]=0,
		[0x66]=8,//backspace
		[0x67]=0,
		[0x68]=0,
		[0x69]=255,
		[0x6a]=0,
		[0x6b]=18,//left arrow
		[0x6c]=255,
		[0x6d]=0,
		[0x6e]=0,
		[0x6f]=0,
		[0x70]=255,
		[0x71]=127,//del
		[0x72]=19,//down arrow
		[0x73]=255,
		[0x74]=20,//right arrow
		[0x75]=17,//up arrow
		[0x76]=27,//esc
		[0x77]=255,
		[0x78]=255,
		[0x79]=43,//+ kp
		[0x7a]=255,
		[0x7b]=45,//- kp
		[0x7c]=42,//* kp
		[0x7d]=255,
		[0x7e]=255,
		[0x7f]=0
	};
	if ((c0 & 0x80)!=0){
		switch (c0){
			default:
			case 0x83:in_flags=0;break;
			case 0xaa:ps2_command(0);break;
			case 0xee:ps2_command(4);break;
			case 0xe1:in_flags |=4;break;
			case 0xe0:in_flags |=2;break;
			case 0xf0:in_flags |=1;break;
			case 0xfa:ps2_command(2);break;
			case 0xfc:
			case 0xfd:ps2_command(1);break;
			case 0xfe:ps2_command(3);break;
		}
		return -2;
	} else {
		c1=(((mod_held & 3)!=0)?byte_to_action1:byte_to_action0)[c0];
	}
	if (c1==255 | c1==0){
		in_flags=0;
	} else if (c1==14){
		mod_held=(in_flags & 1)?(mod_held & 0xfe):(mod_held | 0x01);
		in_flags=0;
	} else if (c1==15){
		mod_held=(in_flags & 1)?(mod_held & 0xfd):(mod_held | 0x02);
		in_flags=0;
	} else {
		if (in_flags & 1){
			in_flags=0;
		} else {
			return c1;
		}
	}
	return -2;
}

int16_t ex_stdin(){
	// returns EOF (-1) when no other characters are avalible (therefore it is non-blocking)
	while (1){
		int16_t v=ps2_read_byte();
		if (v==-2) continue;
		return v;
	}
}

void flasher_switch(){
	uint8_t v1,v2;
	const uint32_t a=0x80800000lu+_terminalCharacterState.cursor*3lu;
	v1=*(volatile uint8_t*)(a+1);
	v2=*(volatile uint8_t*)(a+2);
	*(volatile uint8_t*)(a+1)=v2;
	*(volatile uint8_t*)(a+2)=v1;
}

// waits until ex_stdin() returns non-EOF
// shows cursor position with visual flashing
// performs no lasting visual changes from the character it reads (the caller will have to print the character to the screen if they want it to be shown)
int16_t ex_stdin_block_character(){
	int16_t c;
	*((volatile uint8_t*)(0x80000000lu|0x00lu))=1;
	uint32_t flasher_inc=0;
	bool flasher_state=1;
	flasher_switch();
	while ((c=ex_stdin())==-1){
		if (flasher_inc++>=0x16000){
			flasher_inc=0;
			flasher_state=!flasher_state;
			flasher_switch();
		}
	}
	if (flasher_state){
		flasher_switch();
	}
	*((volatile uint8_t*)(0x80000000lu|0x00lu))=0;
	return c;
}

uint16_t getTerminalCursorLimit();
void _inc_cursor(int m);
void _setchar_screen(uint16_t position, uint8_t character, uint8_t foreground, uint8_t background);
char ex_stdin_line_buffer[256];
int16_t ex_stdin_line_position=-1;
bool stdin_do_line_block;

// reads an entire line and performs visual updating
int16_t ex_stdin_block_line(){
	uint16_t length=0;
	uint16_t position=0;
	ex_stdin_line_buffer[0]=0;
	uint16_t cursorLimit=getTerminalCursorLimit();
	int16_t c=-1;
	while (c!='\n'){
		c=ex_stdin_block_character();
		switch (c){
			case 8:// backspace
			if (position!=0){
				_inc_cursor(-1);
				memmove(ex_stdin_line_buffer+(position-1u),ex_stdin_line_buffer+position,(length-position)+1u);
				length--;
				position--;
			}
			break;
			case 17://up arrow
			// not implemented
			//_inc_cursor(-80);
			break;
			case 18://left arrow
			if (position!=0){
				_inc_cursor(-1);
				position--;
			}
			break;
			case 19:// down arrow
			// not implemented
			//_inc_cursor(80);
			break;
			case 20://right arrow
			if (position<length){
				_inc_cursor(1);
				position++;
			}
			break;
			case '\n':
			if (position!=length) _inc_cursor(length-position);
			position=length;
			break;
			default:
			if (length<253 & c>=' ' & c<='~'){
				_inc_cursor(1);
				memmove(ex_stdin_line_buffer+(position+1u),ex_stdin_line_buffer+position,(length-position)+1u);
				ex_stdin_line_buffer[position]=c;
				length++;
				position++;
			}
		}
		for (uint16_t i=0;i<=length;i++){
			int32_t target=(int32_t)_terminalCharacterState.cursor+((int32_t)i-(int32_t)position);
			if (target>=0 & target<cursorLimit){
				_setchar_screen((uint16_t)target,(i==length)?' ':ex_stdin_line_buffer[i],_terminalCharacterState.current_foreground,_terminalCharacterState.current_background);
			}
		}
		if (c=='\n'){
			_putchar_screen('\n');
			ex_stdin_line_buffer[length++]='\n';
			ex_stdin_line_buffer[length]=0;
			ex_stdin_line_position=1;
			return ex_stdin_line_buffer[0];
		}
	}
}

int16_t ex_stdin_block_appropriate(){
	if (ex_stdin_line_position!=-1){
		if (ex_stdin_line_buffer[ex_stdin_line_position]!=0){
			return ex_stdin_line_buffer[ex_stdin_line_position++];
		} else {
			ex_stdin_line_position=-1;
		}
	}
	if (stdin_do_line_block){return ex_stdin_block_line();} else {return ex_stdin_block_character();}
}

uint16_t getTerminalCursorLimit(){
	uint8_t mode_info=*(volatile uint8_t*)(0x80804ffflu);
	uint8_t font_height=(mode_info &15)+3;
	unsigned n0=480u/(unsigned)font_height;
	if ((mode_info &(1<<4))!=0){
		return n0*80u;
	} else {
		return n0*71u;
	}
}
void _putchar_ensure_cursor_normal(){
	uint8_t mode_info=*(volatile uint8_t*)(0x80804ffflu);
	uint8_t font_height=(mode_info &15)+3;
	unsigned n0=480u/(unsigned)font_height;
	unsigned n1=n0-1;
	unsigned n2;
	if ((mode_info &(1<<4))!=0){
		n2=80;
	} else {
		n2=71;
	}
	n0*=n2;
	n1*=n2;
	while (_terminalCharacterState.cursor>=n0){
		_terminalCharacterState.cursor-=n2;
		uint16_t i;
		for (i=0;i<n1;i++){
			const uint32_t a0=0x80800000lu+(i+ 0)*3lu;
			const uint32_t a1=0x80800000lu+(i+n2)*3lu;
			*(volatile uint8_t*)(a0+0)=*(volatile uint8_t*)(a1+0);
			*(volatile uint8_t*)(a0+1)=*(volatile uint8_t*)(a1+1);
			*(volatile uint8_t*)(a0+2)=*(volatile uint8_t*)(a1+2);
		}
		for (;i<n0;i++){
			const uint32_t a=0x80800000lu+i*3lu;
			*(volatile uint8_t*)(a+0)=' ';
			*(volatile uint8_t*)(a+1)=255;
			*(volatile uint8_t*)(a+2)=0;
		}
	}
}

void _setchar_screen(uint16_t position, uint8_t character, uint8_t foreground, uint8_t background){
	const uint32_t a=0x80800000lu+position*3lu;
	*(volatile uint8_t*)(a+0)=character;
	*(volatile uint8_t*)(a+1)=foreground;
	*(volatile uint8_t*)(a+2)=background;
}

void _putchar_screen(char c){
	if (c>=' ' & c<='~'){
		const uint32_t a=0x80800000lu+_terminalCharacterState.cursor*3lu;
		*(volatile uint8_t*)(a+0)=c;
		*(volatile uint8_t*)(a+1)=_terminalCharacterState.current_foreground;
		*(volatile uint8_t*)(a+2)=_terminalCharacterState.current_background;
		_terminalCharacterState.cursor++;
		_putchar_ensure_cursor_normal();
	} else if (c=='\r'){
		unsigned n=(((*(volatile uint8_t*)(0x80804ffflu)&(1<<4))!=0)?80u:71u);
		_terminalCharacterState.cursor-=_terminalCharacterState.cursor%n;
	} else if (c=='\n'){
		unsigned n=(((*(volatile uint8_t*)(0x80804ffflu)&(1<<4))!=0)?80u:71u);
		_terminalCharacterState.cursor-=_terminalCharacterState.cursor%n;
		_terminalCharacterState.cursor+=n;
		_putchar_ensure_cursor_normal();
	}
}

void _inc_cursor(int m){
	if (m==0) return;
	if (m<0){
		if (_terminalCharacterState.cursor>=-m) _terminalCharacterState.cursor+=m;
	} else {
		_terminalCharacterState.cursor+=m;
		_putchar_ensure_cursor_normal();
	}
}

