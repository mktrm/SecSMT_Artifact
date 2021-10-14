# Author: Kazem Taram

from m5.params import *
from m5.proxy import *
from m5.SimObject import SimObject

class BaseSMTPolicy(SimObject):
    type = 'BaseSMTPolicy'
    abstract = True
    cxx_header = "cpu/smt_policies/base.hh"
    numThreads = Param.Unsigned(Parent.numThreads, "Number of threads")
    size = Param.Unsigned(32, "Size of the resource")

class PartitionedSMTPolicy(BaseSMTPolicy):
    type = 'PartitionedSMTPolicy'
    cxx_class = 'PartitionedSMTPolicy'
    cxx_header = 'cpu/smt_policies/partitioned.hh'


class DynamicSMTPolicy(BaseSMTPolicy):
    type = 'DynamicSMTPolicy'
    cxx_class = 'DynamicSMTPolicy'
    cxx_header = 'cpu/smt_policies/dynamic.hh'

class AdaptiveSMTPolicy(BaseSMTPolicy):
    type = 'AdaptiveSMTPolicy'
    cxx_class = 'AdaptiveSMTPolicy'
    cxx_header = 'cpu/smt_policies/adaptive.hh'
    interval = Param.Unsigned(100, "adaptation interval")
    step = Param.Unsigned(1, "adaptation step")
    limit = Param.Float(1, "maximum ratio of one partitione")
 
class AsymmetricSMTPolicy(BaseSMTPolicy):
    type = 'AsymmetricSMTPolicy'
    cxx_class = 'AsymmetricSMTPolicy'
    cxx_header = 'cpu/smt_policies/asymmetric.hh'
    interval = Param.Unsigned(100, "adaptation interval")
    step = Param.Unsigned(1, "adaptation step")
    limit = Param.Float(1, "maximum ratio of one partitione")
    main_thread = Param.Int(0, "main thread")
    max_borrow =Param.Int(30, "maximum borrowed elements")
    min_free =Param.Int(0, "minimum free left for the other T")