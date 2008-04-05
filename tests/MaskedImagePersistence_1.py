#!/usr/bin/env python
import os

import lsst.afw.image as afwImage
import lsst.daf.base as dafBase
import lsst.daf.persistence as dafPers
import lsst.pex.policy as dafPolicy

# Create the additionalData DataProperty
additionalData = dafBase.DataProperty.createPropertyNode("root")
additionalData.addProperty(dafBase.DataProperty("sliceId", 0))
additionalData.addProperty(dafBase.DataProperty("visitId", "fov391"))
additionalData.addProperty(dafBase.DataProperty("universeSize", 100))
additionalData.addProperty(dafBase.DataProperty("itemName", "foo"))

# Create an empty Policy
policy = dafPolicy.PolicyPtr()

# Get a Persistence object
persistence = dafPers.Persistence.getPersistence(policy)

# Set up the LogicalLocation.  Assumes that previous tests have run, and
# Src_*.fits exists in the current directory.
logicalLocation = dafPers.LogicalLocation("Src")

# Create a FitsStorage and put it in a StorageList.
storage = persistence.getRetrieveStorage("FitsStorage", logicalLocation)
storageList = dafPers.StorageList([storage])

print "Retrieving MaskedImage Src"

# Let's do the retrieval!
maskedImage = afwImage.MaskedImageF.swigConvert( \
    persistence.unsafeRetrieve("MaskedImageF", storageList, additionalData))

# Check the resulting MaskedImage
# ...

print "Persisting MaskedImage as FITS to Dest"

# Persist the MaskedImage (under a different name)
logicalLocation = dafPers.LogicalLocation("Dest")
storage = persistence.getPersistStorage("FitsStorage", logicalLocation)
storageList = dafPers.StorageList([storage])
try:
    persistence.persist(maskedImage, storageList, additionalData)
except dafPolicy.LsstInvalidParameter, e:
    print e.what()
    raise

# Ideally should do a cmp to make sure they are the same, but the persistence
# writes additional comments not present in the original file.
# assert os.system("cmp Src_img.fits Dest_img.fits") == 0
# assert os.system("cmp Src_msk.fits Dest_msk.fits") == 0
# assert os.system("cmp Src_var.fits Dest_var.fits") == 0