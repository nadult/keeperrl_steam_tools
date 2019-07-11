FLAT_HDR=steamworks/public/steam/steam_api_flat.h
sed  -i '/typedef int64 lint64;\|typedef uint64 ulint64;/d' $FLAT_HDR
sed -i 's/\(^[a-zA-Z].* const_k_\)/\/\/ \1/g' $FLAT_HDR
