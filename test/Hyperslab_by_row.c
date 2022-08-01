/************************************************************
  
  This example shows how to write and read a hyperslab.  It 
  is derived from the h5_read.c and h5_write.c examples in 
  the "Introduction to HDF5".

 ************************************************************/
 
#include "hdf5.h"


#define FILE        "sds.h5"
#define DATASETNAME "IntArray" 
#define NX_SUB  3                      /* hyperslab dimensions */ 
#define NY_SUB  5 
#define NX 7                           /* output buffer dimensions */ 
#define NY 7 
#define NZ  3 
#define RANK         2
#define RANK_OUT     2

#define X     5                        /* dataset dimensions */
#define Y     6

int
main (int argc, char **argv)
{
    hsize_t     dimsf[2];              /* dataset dimensions */
    int         data[X][Y];            /* data to write */

    /* 
     * Data  and output buffer initialization. 
     */
    hid_t       file, dataset;         /* handles */
    hid_t       dataspace;   
    hid_t       memspace; 
    hsize_t     dimsm[2];              /* memory space dimensions */
    hsize_t     dims_out[2];           /* dataset dimensions */      
    herr_t      status;                             

    int         data_out[NX][NY]; /* output buffer */
    int data_out1[X][Y];
   
    hsize_t     count[2];              /* size of the hyperslab in the file */
    hsize_t    offset[2];             /* hyperslab offset in the file */
    hsize_t     count_out[2];          /* size of the hyperslab in memory */
    hsize_t    offset_out[2];         /* hyperslab offset in memory */
    int         i, j, k, status_n, rank;
    int print_dbg_msg=1;
    hbool_t op_failed;
    size_t  num_in_progress;
    hid_t   es_id = H5EScreate();
    int provided;
    hid_t	plist_id;                 /* property list identifier */

     /*
     * MPI variables
     */
    int mpi_size, mpi_rank;
    MPI_Comm comm  = MPI_COMM_WORLD;
    MPI_Info info  = MPI_INFO_NULL;

    /*
     * Initialize MPI
     */
    //MPI_Init(&argc, &argv);
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_size(comm, &mpi_size);
    MPI_Comm_rank(comm, &mpi_rank);  
 
    
    
    /* 
     * Set up file access property list with parallel I/O access
     */
     plist_id = H5Pcreate(H5P_FILE_ACCESS);
     H5Pset_fapl_mpio(plist_id, comm, info);
    

    
    /*
     * Create a new file collectively and release property list identifier.
     */
   // file = H5Fcreate(FILE, H5F_ACC_TRUNC, H5P_DEFAULT, plist_id);
    //H5Pclose(plist_id);

/*********************************************************  
   This writes data to the HDF5 file.  
 *********************************************************/  
 if(mpi_rank==0){
    /* 
     * Data  and output buffer initialization. 
     */
    for (j = 0; j < X; j++) {
	for (i = 0; i < Y; i++)
	    data[j][i] = i + j;
    }     
    /*
     * 0 1 2 3 4 5 
     * 1 2 3 4 5 6
     * 2 3 4 5 6 7
     * 3 4 5 6 7 8
     * 4 5 6 7 8 9
     */

    /*
     * Create a new file using H5F_ACC_TRUNC access,
     * the default file creation properties, and the default file
     * access properties.
     */
    
    //file = H5Fcreate (FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);  
    file = H5Fcreate_async (FILE, H5F_ACC_TRUNC, H5P_DEFAULT, plist_id,es_id);  
    H5Pclose(plist_id);

    /*
     * Describe the size of the array and create the data space for fixed
     * size dataset. 
     */
    dimsf[0] = X;
    dimsf[1] = Y;
    dataspace = H5Screate_simple (RANK, dimsf, NULL); 

    /*
     * Create a new dataset within the file using defined dataspace and
     * default dataset creation properties.
     */
    dataset = H5Dcreate_async (file, DATASETNAME, H5T_STD_I32BE, dataspace,
                         H5P_DEFAULT,H5P_DEFAULT, H5P_DEFAULT,es_id);   //H5T_STD_I32BE = 32-bit big-endian signed integers

    /*
     * Write the data to the dataset using default transfer properties.
     */
    status = H5Dwrite_async (dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL,
                      H5P_DEFAULT, data,es_id);  //H5T_NATIVE_INT = C-style int
   
    
   
    status = H5Dread_async (dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL,
                   H5P_DEFAULT, data_out1,es_id);

    
    status = H5ESwait(es_id, H5ES_WAIT_FOREVER, &num_in_progress, &op_failed);
    if (status < 0) {
        fprintf(stderr, "Error with H5ESwait\n");
        
    }
    if (print_dbg_msg)
        fprintf(stderr, "H5ESwait done\n");

    

     printf ("First Data:\n ");
    
    for (j = 0; j < X; j++) {
	for (i = 0; i < Y; i++) printf("%d ", data_out1[j][i]);
	printf("\n ");
    }
	printf("\n");
    /*
     * Close/release resources.
     */
    H5Sclose (dataspace);
   // H5Dclose (dataset);
    status = H5Dclose_async(dataset, es_id);
    if (status < 0) {
        fprintf(stderr, "Closing dataset failed\n");
        //ret = -1;
    }
   
   status = H5Fclose_async(file, es_id);
    if (status < 0) {
        fprintf(stderr, "Closing file failed\n");
        //ret = -1;
    } 
    //H5Fclose(file);
    status = H5ESwait(es_id, H5ES_WAIT_FOREVER, &num_in_progress, &op_failed);
    if (status < 0) {
        fprintf(stderr, "Error with H5ESwait\n");
        //ret = -1;
    } 
}

/*************************************************************  

  This reads the hyperslab from the sds.h5 file just 
  created, into a 2-dimensional plane of the 3-dimensional 
  array.

 ************************************************************/  

    for (j = 0; j < NX; j++) {
	for (i = 0; i < NY; i++) {
	    //for (k = 0; k < NZ ; k++)
		data_out[j][i] = 0;
	}
    } 
 
    /*
     * Open the file and the dataset.
     */
    file = H5Fopen_async (FILE, H5F_ACC_RDONLY, H5P_DEFAULT,es_id); //H5F_ACC_RDONLY= An existing file is opened with read-only access. 
                                                        //If the file does not exist, H5Fopen fails. (Default)
    dataset = H5Dopen_async(file, DATASETNAME,H5P_DEFAULT,es_id);  //#define DATASETNAME "IntArray" 

    dataspace = H5Dget_space_async (dataset,es_id);    /* dataspace handle */
    rank      = H5Sget_simple_extent_ndims (dataspace);
    status_n  = H5Sget_simple_extent_dims (dataspace, dims_out, NULL);
    printf("\nRank: %d\nDimensions: %lu x %lu \n", rank,
	   (unsigned long)(dims_out[0]), (unsigned long)(dims_out[1]));



    dimsm[0] = NX;
    dimsm[1] = NY;  //7x7
   // dimsm[2] = NZ;  //dimsm =7x7
    memspace = H5Screate_simple (RANK_OUT, dimsm, NULL);   //RANK_OUT=2, RANK=2
    /* 
     * Define hyperslab in the dataset. 
     */

     /*
     * 0 1 2 3 4 5 
     * 1 2 3 4 5 6
     * 2 3 4 5 6 7
     * 3 4 5 6 7 8
     * 4 5 6 7 8 9
     */
    if(mpi_rank==0){
    offset[0] = 1;
    offset[1] = 1; // offset=1x1
    count[0]  = NX_SUB;
    count[1]  = NY_SUB; //count=3x5
    status = H5Sselect_hyperslab (dataspace, H5S_SELECT_SET, offset, NULL, count, NULL);  
    /*
     * Define the memory dataspace.
     */
   
    // 7x7x3 array 

    /* 
     * Define memory hyperslab. 
     */
    offset_out[0] = 3;
    offset_out[1] = 2;  // offset_out = 3 X 2
    //offset_out[2] = 0;
    count_out[0]  = NX_SUB; //count_out=3 X 5  
    count_out[1]  = NY_SUB;
    //count_out[2]  = 1;
    //count_out[2]  = 0;
    status = H5Sselect_hyperslab (memspace, H5S_SELECT_SET, offset_out, NULL, 
                                  count_out, NULL);
    
     plist_id = H5Pcreate(H5P_DATASET_XFER);
   H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);
    
    status = H5Dread_async (dataset, H5T_NATIVE_INT, memspace, dataspace,
                      H5P_DEFAULT, data_out,es_id);
    
 
    offset[0] = 1;
    offset[1] = 0; // offset=1x0
    count[0]  = 4;
    count[1]  = 1; //count=4x1
    status = H5Sselect_hyperslab (dataspace, H5S_SELECT_SET, offset, NULL, count, NULL);  
    
    offset_out[0] = 3;
    offset_out[1] = 1;  // offset_out = 3X 1 
    //offset_out[2] = 0;
    count_out[0]  = 4; //count_out= 4X 1  
    count_out[1]  = 1;
    //count_out[2]  = 1;
    //count_out[2]  = 0;
    status = H5Sselect_hyperslab (memspace, H5S_SELECT_SET, offset_out, NULL, 
                                  count_out, NULL);
    /*
     * Read data from hyperslab in the file into the hyperslab in 
     * memory and display.
     */
    status = H5Dread_async (dataset, H5T_NATIVE_INT, memspace, dataspace,
                      H5P_DEFAULT, data_out,es_id);
    
    }

    if(mpi_rank==1){
    //bottom right corner
 
    offset[0] = 4;
    offset[1] = 1; // offset=4x1
    count[0]  = 1;
    count[1]  = 5; //count=1x5
    status = H5Sselect_hyperslab (dataspace, H5S_SELECT_SET, offset, NULL, count, NULL);  
    
    offset_out[0] = 6;
    offset_out[1] = 2;  // offset_out = 6X 2 
    //offset_out[2] = 0;
    count_out[0]  = 1; //count_out= 1X 5  
    count_out[1]  = 5;
    //count_out[2]  = 1;
    //count_out[2]  = 0;
    status = H5Sselect_hyperslab (memspace, H5S_SELECT_SET, offset_out, NULL, 
                                  count_out, NULL);
    /*
     * Read data from hyperslab in the file into the hyperslab in 
     * memory and display.
     */
    status = H5Dread_async (dataset, H5T_NATIVE_INT, memspace, dataspace,
                      H5P_DEFAULT, data_out,es_id);
    
        
    
    //top left corner
    
    offset[0] = 0;
    offset[1] = 0; // offset=0x0
    count[0]  = 1;
    count[1]  = 6; //count=1x6
    status = H5Sselect_hyperslab (dataspace, H5S_SELECT_SET, offset, NULL, count, NULL);  
    
    offset_out[0] = 2;
    offset_out[1] = 1;  // offset_out = 2X 1 
    //offset_out[2] = 0;
    count_out[0]  = 1; //count_out= 1X 6  
    count_out[1]  = 6;
    //count_out[2]  = 1;
    //count_out[2]  = 0;
    status = H5Sselect_hyperslab (memspace, H5S_SELECT_SET, offset_out, NULL, 
                                  count_out, NULL);
    /*
     * Read data from hyperslab in the file into the hyperslab in 
     * memory and display.
     */
    status = H5Dread_async (dataset, H5T_NATIVE_INT, memspace, dataspace,
                      H5P_DEFAULT, data_out,es_id);
    }
    status = H5ESwait(es_id, H5ES_WAIT_FOREVER, &num_in_progress, &op_failed);
    if (status < 0) {
        fprintf(stderr, "Error with H5ESwait\n");
        
    }
    if (print_dbg_msg)
        fprintf(stderr, "H5ESwait done\n");
  
   //if(mpi_rank==1){
    printf ("Data:\n ");
    for (j = 0; j < NX; j++) {
	for (i = 0; i < NY; i++) printf("%d ", data_out[j][i]);
	printf("\n ");
    }
	printf("\n");
   //}
    /*
     * 0 0 0 0 0 0 0
     * 0 0 0 0 0 0 0
     * 0 0 0 0 0 0 0
     * 3 4 5 6 0 0 0  
     * 4 5 6 7 0 0 0
     * 5 6 7 8 0 0 0
     * 0 0 0 0 0 0 0
     */
    
    
    //H5ESclose(es_id);
    
    /*
     * Close and release resources.
     */
    //H5Dclose(dataset);
    
    //H5Dclose_async(dataset,es_id);
    status = H5Dclose_async(dataset, es_id);
    if (status < 0) {
        fprintf(stderr, "Closing dataset failed\n");
        //ret = -1;
    }
    /* status = H5ESwait(es_id, H5ES_WAIT_FOREVER, &num_in_progress, &op_failed);
    if (status < 0) {
        fprintf(stderr, "Error with H5ESwait\n");
        //ret = -1;
    } */
    H5Sclose (dataspace);
    H5Sclose (memspace);

    //H5Fclose_async(file,es_id);
    status = H5Fclose_async(file, es_id);
    if (status < 0) {
        fprintf(stderr, "Closing file failed\n");
        //ret = -1;
    }
    
    status = H5ESwait(es_id, H5ES_WAIT_FOREVER, &num_in_progress, &op_failed);
    if (status < 0) {
        fprintf(stderr, "Error with H5ESwait\n");
        //ret = -1;
    }
    status = H5ESclose(es_id);
    if (status < 0) {
        fprintf(stderr, "Can't close second event set\n");
        //ret = -1;
    }
    
    MPI_Finalize();

}     

