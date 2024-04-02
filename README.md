Υλοποίηση Επεκτατού Κατακερματισμού σε σύνολο εγγραφών

Οδηγίες εκτέλεσης:

	Για την δημιουργία του εκτελέσιμου με τα tests:
		make ht

	Για να τρέξετε τα tests:
		make run

Νέες δομές, συναρτήσεις, σταθερές:

	Πέραν των δωσμένων δομών, ορίσαμε:
		Τη δομή ht_info η οποία περιέχει τις εξής πληροφορίες για το αρχείο:
			Συνολικός αριθμός εγγραφών, ολικό βάθος, τύπος αρχείου(heap ή hash), δείκτης προς τον πίνακα κατακερματισμού.
		Τη δομή ht_block_info η οποία περιέχει τις εξής πληροφορίες για το block:
			Αριθμό εγγραφών, τοπικό βάθος.
		Τη δομή table_file_entry η οποία χρησιμοποιείται στον πίνακα με τα ανοιχτά αρχεία και περιέχει:
			Το όνομα του αρχείου, το αναγνωριστικό του αρχείου(fileDesc).

	Ορίσαμε τη σταθερά RECORDS_PER_BLOCK, η οποία ισούται με [(BF_BLOCK_SIZE - sizeof(HT_block_info)) / sizeof(Record)],
	δηλαδή το μέγεθος του block χωρίς τη δομή ht_block_info δια το μέγεθος της κάθε εγγραφής.

	Ορίσαμε τις εξής βοηθητικές συναρτήσεις:

		HT_info *getInfo(int indexDesc):
			Η συνάρτηση φέρνει στη μνήμη το block μεταδεδομένων του αρχείου που βρίσκεται στη θέση indexDesc του πίνακα
			ανοιχτών αρχείων και επιστρέφει έναν δείκτη στο block αυτό.

		int hash_Function(int num):
			Η συνάρτηση κατακερματισμού που έχουμε χρησιμοποιήσει είναι αρκετά απλή

		void reHash(int fileDesc, int oldBlockPos, int newBlockPos, int *hashTable, int globalDepth):
			Η συνάρτηση αυτή παίρνει τις θέσεις από δύο blocks που ήταν φιλαράκια, το αναγνωριστικό του αρχείου, δείκτη στον 
			πίνακα κατακερματισμού και το ολικό βάθος και εφαρμόζει τη συνάρτηση κατακερματισμού για κάθε εγγραφή με σκοπό την 
			ανακατανομή των εγγραφών στα blocks.

		unsigned int getMSBs(unsigned int num, int depth):
			Η συνάρτηση αυτή δέχεται ως όρισμα έναν αριθμό και έναν αριθμό ψηφίων depth και επιστρέφει τα depth πιο σημαντικά
			bits του αριθμού αυτού.

		void resizeHashTable(HT_info *info):
			Η συνάρτηση αυτή διπλασιάζει τον πίνακα κατακερματισμού τον οποίο παίρνει μέσα από το HT_info το οποίο στο τέλος 
			ενημερώνει με τον νέο πίνακα. Για τον διπλασιασμό, δεσμεύουμε την κατάλληλη μνήμη και για κάθε θέση i του παλιού 
			πίνακα η οποία δείχνει σε μία θέση x, οι θέσεις (2*i) και (2*i + 1) θα δείχνουν στη θέση x.

Συναρτήσεις βιβλιοθήκης HT:
	Οι συναρτήσεις λειτουργούν ακριβώς όπως περιγράφονται στην εκφώνηση της εργασίας. Ακολουθούν επιπλέον διευκρινήσεις και 
	σχεδιαστικές επιλογές:

	HT_ErrorCode HT_Init(): 
		H συνάρτηση δεσμεύει μνήμη για τον πίνακα ανοιχτών αρχείων (αντικείμενα της δομής table_file_entry) ο οποίος
		αποθηκεύεται σε global μεταβλητή και αρχικοποιεί σε 0 την, επίσης global μεταβλητή, openFileCounter ή οποία
		 αναπαριστά των αριθμό των ανοιχτών αρχείων ανά πάσα στιγμή.

	HT_ErrorCode HT_Destroy():
		Η συνάρτηση καλεί την HT_CloseFile() για κάθε θέση του πίνακα που έχει ανοιχτό αρχείο. Στη συνέχεια αποδεσμεύει
		τη μνήμη που καταλαμβάνει ο πίνακας.

	HT_ErrorCode HT_CreateIndex(const char *filename, int depth): 
		Η συνάρτηση δημιουργεί, μαζί με το αρχείο κατακερματισμού και ένα αρχείο με όνομα "dict[fileName]" στο οποίο θα 
		αποθηκεύεται το ευρετήριο του αρχείου. Μαζί με τη δημιουργία του αρχείου κατακερματισμού, γίνεται και μία αρχικοποίηση
		του HT_Info block.
	
	HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc): 
		Στην αρχή γίνεται μέσω της fopen έλεγχος ότι υπάρχει αρχείο με το δοθέν όνομα. Σε αντίθετη περίπτωση, επιστρέφεται
		HT_Error, όπως και στην περίπτωση που έχουμε ήδη MAX_OPEN_FILES ανοιχτά αρχεία. Μαζί με το αρχείο, ανοίγουμε και το 
		αρχείο του ευρετηρίου. Αν αυτό έχει ήδη δεδομένα, αν δηλαδή υπάρχει ήδη ο πίνακας, αφού δεσμεύσουμε τη μνήμη που 
		χρειαζόμαστε για αυτόν, αντιγράφουμε τα δεδομένα από τα blocks στις θέσεις του πίνακα. Στη συνέχεια, κλείνουμε το αρχείο
		με το ευρετήριο. Θα το ανοίξουμε ξανά κατά το κλείσιμο του αρχείου για να το ενημερώσουμε.
	
	HT_ErrorCode HT_CloseFile(int indexDesc):
		Στην αρχή, ελέγχουμε ότι στη θέση indexDesc υπάρχει όντως ανοιχτό αρχείο. Σε αντίθετη περίπτωση, επιστρέφεται HT_Error.
		Ανοίγουμε το αρχείο με το ευρετήριο και αντιγράφουμε τον πίνακα κατακερματισμού. Για κάθε block που χρειαζόμαστε, αν 
		αυτό ήδη υπάρχει στο αρχείο, κάνουμε απλώς overwrite τα δεδομένα του. Αν πρόκειται για νέα blocks, κάνουμε allocate και
		αντιγράφουμε τα δεδομένα στο νέο block. Τέλος, κλείνουμε και τα δύο αρχεία.
	
	HT_ErrorCode HT_InsertEntry(int indexDesc, Record record):
		Για την εισαγωγή της εγγραφής στο αρχείο, βρίσκουμε την τιμή της συνάρτησης κατακερματισμού και τα αντίστοιχα
		ψηφία ανάλογα με το ολικό βάθος και διακρίνουμε περιπτώσεις για τη θέση του πίνακα στην οποία καταλήγουμε: 

			Σε περίπτωση που το bucket δεν έχει ακόμη δημιουργηθεί, κάνουμε allocate νέο block στο αρχείο. Ορίζουμε τη θέση 
			του πίνακα ίση με τη θέση του block στο αρχείο. Φτιάχνουμε τα δεδομένα για το ht_block_info και στη συνέχεια
			αρχικοποιούμε τα φιλαράκια της θέσεις.

			Αν το block υπάρχει και υπάρχει χώρος σε αυτό μπορούμε απλώς να εισάγουμε την εγγραφή.

			Αν το block υπάρχει και δεν υπάρχει χώρος διακρίνουμε ξανά δύο περιπτώσεις. Την περίπτωση το τοπικό βάθος να είναι
			μικρότερο από το ολικό και την περίπτωση να είναι ίσα. Στην περίπτωση που είναι ίσα, χρειάζεται πρώτα να διπλασιάσουμε 
			τον πίνακα κατακερματισμού και συνεπώς να αυξήσουμε το ολικό βάθος. Αυτό μας φέρνει στην επόμενη περίπτωση, όπου το
			bucket υπάρχει, δεν έχει χώρο για νέα εγγραφή, όμως το τοπικό βάθος είναι μικρότερο από το ολικό βάθος. Σε αυτή την 
			περίπτωση, βρίσκουμε όλα τα φιλαράκια της θέσης, και τα σπάμε σε δύο blocks, το ήδη υπάρχον και ένα νέο που κάνουμε
			allocate εκείνη την ώρα. Καλούμε τη συνάρτηση reHash για τα δύο blocks αυτά για την ανακατανομή των εγγραφών. Στο 
			σημείο αυτό, υπάρχει περίπτωση ακόμη να μην μπορεί να γίνει η εισαγωγή αν οι εγγραφές έχουν κάνει hash ξανά στο ίδιο
			block, οπότε για την ορθή λειτουργία, χρειάζεται να καλέσουμε ξανά την HT_InsertEntry() αναδρομικά.
	
	HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id):
		Στην περίπτωση που δίνεται NULL αντί για κάποιο id, παίρνουμε όλα τα blocks και στη συνέχεια όλα τα records με τη σειρά
		και εκτυπώνουμε κάθε record. Εάν δοθεί κάποιο id, χρησιμοποιούμε τη συνάρτηση κατακερματισμού για να βρούμε το αντίστοιχο
		block. Παίρνουμε με τη σειρά τις εγγραφές και εκτυπώνουμε όσες έχουν id ίσο με το δοθέν. Έχει γίνει η παραδοχή ότι όλες οι
		εγγραφές με το ίδιο id χωράνε στο ίδιο block. 

	HT_ErrorCode HashStatistics(char* filename):
		Έχουμε κάνει την παραδοχή ότι θα πρέπει το αρχείο να είναι ανοιχτό για τη συγκεκριμένη συνάρτηση. Στην αρχή της 
		συνάρτησης, αν δεν βρεθεί στον πίνακα με τα ανοιχτά αρχεία το αρχείο με όνομα filename επιστρέφεται HT_Error. Εφόσον 
		βρεθεί, ορίζουμε τρεις μεταβλητές για τις μέγιστες εγγραφές σε ένα block, τις ελάχιστες εγγραφές σε ένα block και τις 
		συνολικές εγγραφές. Διασχίζουμε όλα τα blocks του αρχείου και ενημερώνουμε τις μεταβλητές κατάλληλα. Στο τέλος, 
		εκτυπώνουμε τα αποτελέσματα.

Πληροφορίες για τα tests: 

	Αν τα tests τρέξουν πάνω από μία φορά, θα αποτυγχάνει το test της HT_CreateIndex() καθώς το αρχείο υπάρχει ήδη. Αυτό 
	δεν μπορεί να αποφευχθεί καθώς θα αποτύγχαναν όλα.

	Η αποθήκευση του ευρετηρίου ελέγχεται μέσα στην test HT_InsertEntry() όπου το αρχείο κλείνει και ανοίγει ξανά.
