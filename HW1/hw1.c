#include<stdio.h>
#include <mpi.h>
#include <stdlib.h>

int main(int argc ,char *argv[]) {

// setup MPI Environment
    MPI_Status status;
    MPI_Request *request_R ,request_S;

//Init MPI
    MPI_Init(&argc ,&argv);

    int rankID;
    int rankTotal;

//Get process number and process ID
    MPI_Comm_size(MPI_COMM_WORLD ,&rankTotal);
    MPI_Comm_rank(MPI_COMM_WORLD ,&rankID);

//Check the number of processor
    if ( rankTotal != 4 ) {
        printf("The number of processor must be 4");
        MPI_Finalize();
        return 0;
    }

//Calculate the row numbers of each rank
    int rowPerRank = 10000 / rankTotal;

//create buffer
    int buffer[rowPerRank][10000];


//create space to save requests for receive
    request_R = ( MPI_Request * ) malloc(rowPerRank * sizeof(MPI_Request));

//Start Receiving
    receiveMsg(rowPerRank ,rankID ,buffer ,request_R);

//generating array while send it.
    if ( rankID == 0 ) {
        generateAndSend(rowPerRank ,rankTotal ,rankID ,&request_S);
    }

//calculate sum and display
    calculateRowSum(rowPerRank ,request_R ,&status ,buffer ,rankID);


//finalize the MPI environment
    MPI_Finalize();
    free(request_R);
	return 0;
}

void receiveMsg(int rowCountPP ,int rankID ,int msgbuf[][10000] ,MPI_Request *requestList) {
    int i;
    for ( i = 0; i < rowCountPP; i++ ) {
        int rowID = rankID * rowCountPP + i;
        MPI_Irecv(&msgbuf[ i ] ,10000 ,MPI_INT ,0 ,rowID ,MPI_COMM_WORLD ,&( requestList[ i ] ));
//printf("Process #%d receives row #%d.\n", rankID, rowCountPP);
    }
}

void generateAndSend(int rowCountPP ,int rankTotal ,int rankID ,MPI_Request *requestSend) {
    int array[10000][10000];
    int receiveRankID = 0;
    int rowID = 0;
    int colID = 0;

    while ( receiveRankID < rankTotal ) {
//Generate Numbers of one row
        for ( colID = 0; colID < 10000; colID++ ) {
            int random = rand() % 10 + 1;
            array[ rowID ][ colID ] = random;
        }

//Send this row to receive rank
        MPI_Isend(&array[ rowID ] ,10000 ,MPI_INT ,receiveRankID ,rowID ,MPI_COMM_WORLD ,requestSend);
//printf("Row #%d is sent to process #%d.\n", rowID, receiveRankID);

        rowID++;
        if ( rowID % rowCountPP == 0 ) {
            receiveRankID++;
        }
    }
}

int computeRowSum(int *arr) {
    int sumRow = 0;
    int i;
    for ( i = 0; i < 10000; i++ ) {
        sumRow = sumRow + arr[ i ];
    }
    return sumRow;
}

void calculateRowSum(int rowPerRank ,MPI_Request *request_R ,MPI_Status *status ,int msgbuf[][10000] ,int rankID) {
    int rowID = 0;
    long rankSum[4];
    long buffer_long[4];

    while ( rowID < rowPerRank ) {
        int flag = 0;

        if ( request_R[ rowID ] == NULL ) {
            continue;
        }

        while ( flag == 0 ) {
            MPI_Test(&( request_R[ rowID ] ) ,&flag ,status);
        }

        request_R[ rowID ] = NULL;
        long sumRow = computeRowSum(msgbuf[ rowID ]);
        long sumRank = sumRank + sumRow;
        rankSum[rankID] = sumRank;

        int rowID_true = rankID * rowPerRank + rowID;
        //printf("The sum of row #%d on process #%d is %ld.\n" ,rowID_true ,rankID ,sumRow);
        rowID++;
    }

    printf("The sum of  process #%d is %ld.\n" ,rankID ,rankSum[rankID]);

    if (rankID != 0){
        MPI_Send(&rankSum[rankID], 1, MPI_LONG, 0, rankID, MPI_COMM_WORLD);
        printf("send from  process #%d is %ld.\n" ,rankID ,rankSum[rankID]);
    }
// receive and calculate the sum of matrix
    if ( rankID == 0 ) {
        MPI_Recv(&buffer_long[1], 1,MPI_LONG, 1, 1, MPI_COMM_WORLD,MPI_STATUS_IGNORE );
        //printf("RRR %ld.\n" ,buffer_long[1]);
        MPI_Recv(&buffer_long[2], 1,MPI_LONG, 2, 2, MPI_COMM_WORLD,MPI_STATUS_IGNORE );
        //printf("RRR %ld.\n" ,buffer_long[2]);
        MPI_Recv(&buffer_long[3], 1,MPI_LONG, 3, 3, MPI_COMM_WORLD,MPI_STATUS_IGNORE );
        //printf("RRR %ld.\n" ,buffer_long[3]);

        long sum_m = rankSum[0] + buffer_long[1] + buffer_long[2] + buffer_long[3];

        printf("The sum of  matrix is %ld.\n" ,sum_m);
    }
}