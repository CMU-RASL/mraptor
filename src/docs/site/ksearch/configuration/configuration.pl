####INDEXER CONFIGURATION############################
# Where you want the indexer to start (full path; end with /)
$INDEXER_START = '/usr0/webguy/htdocs/brennan/mraptor/';

# The minimum length of a term to index. 
$MIN_TERM_LENGTH = 2;

# The maximum length of a term to index (prevents indexing very long strings).
$MAX_TERM_LENGTH = 40;

# How many words should be used for the description (may be shorter for meta descriptions)
$DESCRIPTION_LENGTH = 15;

# Set this to the term number (first term is 0) you want to start your description at
# This allows you to skip common headers in the body and show meaningful content
# This does not change meta descriptions
$DESCRIPTION_START = 7;

# Which extensions should be indexed
@FILE_EXTENSIONS = ("html", "htm", "shtml");

# Set this to 0 if you do not want to index whole numbers
$INDEX_NUMBERS = 1;

# If you want to remove common terms from the index that are not in the STOP TERMS file
# set this value to the maximum percentage of files that indexed terms can exist in or set it to 0
# For example: if set to 90, terms that exist in less than 90 percent of all files will only be indexed
# This option may increase the search speed by ignoring common terms but may give less accurate results
$REMOVE_COMMON_TERMS = 100;

# Set this to 1 if you want to create a log file of the indexing routine (saved as log.txt)
# This logs information about each file indexed and the indexer configuration 
$MAKE_LOG = 1;

####SEARCH ENGINE CONFIGURATION#############################
# The base url of your site being indexed 
# Be sure it corresponds to the $INDEXER_START directory (end with /)
$BASE_URL = 'http://gs295.sp.cs.cmu.edu/brennan/mraptor/';
# This will be appended to BASE_URL, except for index.html
$PAGE_REF = 'index.html?page=';

# The full-path of the Ksearch Client Side directory (end with /)
$KSEARCH_DIR = '/usr0/webguy/htdocs/brennan/mraptor/ksearch/';

# How many results should be shown per page 
$RESULTS_PER_PAGE = 10;

# Set this to 0 if you do not want to display the file modification dates in the results
$DISPLAY_UPDATE = 1;

# Set this to 0 if you do not want to display the file size in the results
$DISPLAY_SIZE = 1;


#### End of configuration settings #####################################
#### You do not have to edit below #####################################
$CONFIGURATION_DIR = $KSEARCH_DIR.'configuration/'; 

# The full-path of the ksearch searchtopframe.html
#$TOP_FRAME = $KSEARCH_DIR.'searchtopframe.html';
$TOP_FRAME = $KSEARCH_DIR.'search.js';

# The url of the (CSS) style sheet for the results page
$STYLE_SHEET = "style.css";

$IGNORE_FILES_FILE = $CONFIGURATION_DIR.'ignore_files.txt'; 
$STOP_TERMS_FILE = $CONFIGURATION_DIR.'stop_terms.txt';
$DATABASE = $KSEARCH_DIR.'database.js';
$LOG_FILE = $KSEARCH_DIR.'log.txt';
$VERSION = "1.0";

1;
