import twitter
import redis
import sys
import getopt
import time
import json
from functools import partial
from urllib2 import URLError
from httplib import BadStatusLine


def oauth_login(oauth_token, oauth_token_secret, consumer_key, consumer_secret):
    
    # XXX: Go to http://twitter.com/apps/new to create an app and get values
    # for these credentials that you'll need to provide in place of these
    # empty string values that are defined as placeholders.
    # See https://dev.twitter.com/docs/auth/oauth for more information 
    # on Twitter's OAuth implementation.
    
    auth = twitter.oauth.OAuth(oauth_token, oauth_token_secret,
                               consumer_key, consumer_secret)
    
    twitter_api = twitter.Twitter(auth=auth)
    return twitter_api


def make_twitter_request(twitter_api_func, max_errors=100, *args, **kw): 
    
    # A nested helper function that handles common HTTPErrors. Return an updated
    # value for wait_period if the problem is a 500 level error. Block until the
    # rate limit is reset if it's a rate limiting issue (429 error). Returns None
    # for 401 and 404 errors, which requires special handling by the caller.
    
    def handle_twitter_http_error(e, wait_period=2, sleep_when_rate_limited=True):
    
        if wait_period > 3600: # Seconds
            print >> sys.stderr, 'Too many retries. Quitting.'
            raise e
    
        # See https://dev.twitter.com/docs/error-codes-responses for common codes
    
        if e.e.code == 401:
            print >> sys.stderr, 'Encountered 401 Error (Not Authorized)'
            return None
        elif e.e.code == 404:
            print >> sys.stderr, 'Encountered 404 Error (Not Found)'
            return None
        elif e.e.code == 429: 
            print >> sys.stderr, 'Encountered 429 Error (Rate Limit Exceeded)'
            if sleep_when_rate_limited:
                print >> sys.stderr, "Retrying in 15 minutes...ZzZ..."
                sys.stderr.flush()
                time.sleep(60*15 + 5)
                print >> sys.stderr, '...ZzZ...Awake now and trying again.'
                return 2
            else:
                raise e # Caller must handle the rate limiting issue
        elif e.e.code in (500, 502, 503, 504):
            print >> sys.stderr, 'Encountered %i Error. Retrying in %i seconds' % \
                (e.e.code, wait_period)
            time.sleep(wait_period)
            wait_period *= 1.5
            return wait_period
        else:
            raise e

    # End of nested helper function
    
    wait_period = 2 
    error_count = 0 

    while True:
        try:
            return twitter_api_func(*args, **kw)
        except twitter.api.TwitterHTTPError, e:
            error_count = 0 
            wait_period = handle_twitter_http_error(e, wait_period)
            if wait_period is None:
                return
        except URLError, e:
            error_count += 1
            print >> sys.stderr, "URLError encountered. Continuing."
            if error_count > max_errors:
                print >> sys.stderr, "Too many consecutive errors...bailing out."
                raise
        except BadStatusLine, e:
            error_count += 1
            print >> sys.stderr, "BadStatusLine encountered. Continuing."
            if error_count > max_errors:
                print >> sys.stderr, "Too many consecutive errors...bailing out."
                raise


def get_friends_followers(twitter_api, screen_name=None, user_id=None,
                              friends_limit=sys.maxint, followers_limit=sys.maxint):
    
    assert (screen_name != None) != (user_id != None), \
    "Must have screen_name or user_id, but not both"
    
    get_friends = partial(make_twitter_request, twitter_api.friends.list, 
                              count=200)
    get_followers = partial(make_twitter_request, twitter_api.followers.list, 
                                count=200)

    friends_info, followers_info = [], []
    
    for twitter_api_func, limit, info, label in [
                    [get_friends, friends_limit, friends_info, "friends"], 
                    [get_followers, followers_limit, followers_info, "followers"]
                ]:
        
        if limit == 0: continue
        
        cursor = -1
        while cursor != 0:
        
            # Use make_twitter_request via the partially bound callable...
            if screen_name: 
                response = twitter_api_func(screen_name=screen_name, cursor=cursor)
            else: # user_id
                response = twitter_api_func(user_id=user_id, cursor=cursor)

            if response is not None:
                info += response['users']
                cursor = response['next_cursor']
        
            print >> sys.stderr, 'Fetched {0} total {1} for {2}'.format(len(info), 
                                                    label, (user_id or screen_name))
        
            # XXX: You may want to store data during each iteration to provide an 
            # an additional layer of protection from exceptional circumstances
        
            if len(info) >= limit or response is None:
                break

    # Do something useful with the profiles, like store them to disk...
    return friends_info[:friends_limit], followers_info[:followers_limit]


def get_reciprocal_friends(twitter_api, screen_name=None, user_id=None, limit=sys.maxint):

    # Return a list containing user_id's most pupolar reciprocal friends

    friends_info, followers_info = get_friends_followers(twitter_api, screen_name=screen_name, user_id=user_id)
    friends_dict = dict([(info['id'], info) for info in friends_info])
    followers_dict = dict([(info['id'], info) for info in followers_info])
    
    intersect = set(friends_dict.keys()).intersection(set(followers_dict.keys()))
    rank = sorted([(friends_dict[uid]['followers_count'], friends_dict[uid]) for uid in intersect])

    return [info for (count, info) in rank[:limit]]    

def get_twitter_network(twitter_api, redis_cli, screen_name, limit=3):
    
    if redis_cli.exists('list@' + screen_name):
        return
    marked = set([])
    info = make_twitter_request(twitter_api.users.show, screen_name=screen_name)
    
    queue = get_reciprocal_friends(twitter_api, screen_name)
    next_queue = []
    marked.add(info['id'])
    for key in info:
        redis_cli.hset('info@' + screen_name, key, info[key])
    for finfo in queue:
        redis_cli.rpush('list@' + screen_name, finfo['screen_name'])
        for key in finfo:
            redis_cli.hset('info@' + finfo['screen_name'], key, finfo[key])
    
    for i in range(0, limit - 1):
        for finfo in queue:
            if finfo['id'] in marked or redis_cli.exists('list@' + finfo['screen_name']):
                continue
            marked.add(finfo['id'])

            fqueue = get_reciprocal_friends(twitter_api, finfo['screen_name'])
            for ffinfo in fqueue:
                redis_cli.rpush('list@' + finfo['screen_name'], ffinfo['screen_name'])
                if not redis_cli.exists('info@' + ffinfo['screen_name']):
                    for key in ffinfo:
                        redis_cli.hset('info@' + ffinfo['screen_name'], key, ffinfo[key])
                                               
            next_queue += fqueue
                        
        (queue, next_queue) = (next_queue, [])

def crawler(twitter_api, host, port):
    redis_cli = redis.StrictRedis(host=host, port=port, db=0)
    while True:
        name = redis_cli.lpop('update@request')
        if name:
            get_twitter_network(twitter_api, redis_cli, name, 1)
            redis_cli.rpush('update@reply', name)
        time.sleep(0.1)

def main(argv):
    keyfile = ''
    host = ''
    port = 0
    
    try:
        opts, args = getopt.getopt(argv,'k:h:p:',['keyfile=','host=','port=','help'])
    except getopt.GetoptError:
        print 'crawler.py -k <keyfile> -h <host> -p <port>'
        sys.exit(1)
    for opt, arg in opts:
        if opt in ('-k', '--keyfile'):
            keyfile = arg
        elif opt in ('-h', '--host'):
            host = arg
        elif opt in ('-p', '--port'):
            port = arg
        elif opt == '--help':
            print 'crawler.py -k <keyfile> -h <host> -p <port>'
            sys.exit()

    try:
        with open(keyfile) as file:
            keys = [line for line in file]
            if len(keys) < 4:
                print 'Incorrect keyfile.'
                sys.exit(2)

            oauth_token = keys[0].strip()
            oauth_token_secret = keys[1].strip()
            consumer_key = keys[2].strip()
            consumer_secret = keys[3].strip()
            twitter_api = oauth_login(oauth_token, oauth_token_secret, consumer_key, consumer_secret)
    except IOError:
        print 'Cannot find file: ', keyfile
        sys.exit(3)

    crawler(twitter_api, host, port)
        
if __name__ == '__main__':
    main(sys.argv[1:])
