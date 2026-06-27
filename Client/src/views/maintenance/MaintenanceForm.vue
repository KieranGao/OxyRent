<template>
  <div class="page-container centered">
    <div class="glass-card" style="max-width: 480px; width: 100%;">
      <div class="glass-card-header">
        <h3>{{ isEdit ? '编辑维保记录' : '添加维保记录' }}</h3>
      </div>
      <div class="glass-card-body padded">
      <el-form
        ref="formRef"
        :model="form"
        :rules="rules"
        label-position="top"
      >
        <el-form-item v-if="!isEdit" label="选择车辆" prop="vehicle_id">
          <el-select v-model="form.vehicle_id" placeholder="请选择需要维护的车辆" style="width: 100%" filterable>
            <el-option v-for="v in vehicles" :key="v.id" :label="`${v.plate_number} - ${v.brand} ${v.model}`" :value="v.id" />
          </el-select>
        </el-form-item>

        <el-form-item label="类型" prop="type">
          <el-select v-model="form.type" placeholder="请选择类型" style="width: 100%" :disabled="isEdit">
            <el-option label="常规保养" value="maintenance" />
            <el-option label="维修" value="repair" />
          </el-select>
        </el-form-item>

        <el-form-item label="描述" prop="description">
          <el-input v-model="form.description" type="textarea" :rows="3" placeholder="请描述维保内容" :disabled="isEdit" />
        </el-form-item>

        <div class="form-row">
          <el-form-item label="费用 (CNY)" style="flex: 1">
            <el-input-number v-model="form.cost" :min="0" :precision="2" style="width: 100%" />
          </el-form-item>
          <el-form-item label="技师" style="flex: 1">
            <el-input v-model="form.technician" placeholder="技师姓名" />
          </el-form-item>
        </div>

        <div class="form-row">
          <el-form-item label="开始日期" style="flex: 1">
            <el-date-picker v-model="form.start_date" type="date" placeholder="开始日期" value-format="YYYY-MM-DD" style="width: 100%" />
          </el-form-item>
          <el-form-item v-if="isEdit" label="结束日期" prop="end_date" style="flex: 1">
            <el-date-picker v-model="form.end_date" type="date" placeholder="结束日期" value-format="YYYY-MM-DD" style="width: 100%" />
          </el-form-item>
        </div>

        <el-form-item v-if="isEdit" label="状态">
          <el-select v-model="form.status" placeholder="请选择状态" style="width: 100%">
            <el-option label="待处理" value="pending" />
            <el-option label="进行中" value="in_progress" />
            <el-option label="已完成" value="completed" />
          </el-select>
        </el-form-item>

        <el-form-item>
          <el-button type="primary" :loading="submitting" @click="handleSubmit">
            {{ isEdit ? '保存修改' : '创建记录' }}
          </el-button>
          <el-button @click="$router.back()">取消</el-button>
        </el-form-item>
      </el-form>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, computed, onMounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { createMaintenance, updateMaintenance, getMaintenanceList } from '@/api/maintenance'
import { getVehicleList } from '@/api/vehicle'
import { ElMessage } from 'element-plus'

const route = useRoute()
const router = useRouter()
const isEdit = computed(() => !!route.params.id)
const submitting = ref(false)
const formRef = ref(null)
const vehicles = ref([])

const form = reactive({
  vehicle_id: undefined,
  type: '',
  description: '',
  cost: 0,
  technician: '',
  start_date: '',
  end_date: '',
  status: 'pending',
})

const rules = computed(() => {
  const baseRules = {
    type: [{ required: true, message: '请选择类型', trigger: 'change' }],
    description: [{ required: true, message: '请输入描述', trigger: 'blur' }],
  }
  if (!isEdit.value) {
    baseRules.vehicle_id = [{ required: true, message: '请选择车辆', trigger: 'change' }]
  } else {
    baseRules.end_date = [{ required: true, message: '请填入结束日期', trigger: 'change' }]
  }
  return baseRules
})

async function loadRecord() {
  if (!isEdit.value) return
  try {
    const res = await getMaintenanceList({ page: 1, page_size: 100 })
    if (res.error === 0) {
      const record = (res.list || res.records || []).find(r => String(r.id) === String(route.params.id))
      if (record) {
        Object.assign(form, {
          vehicle_id: record.vehicle_id || undefined,
          type: record.type || '',
          description: record.description || '',
          cost: record.cost || 0,
          technician: record.technician || '',
          start_date: record.start_date || '',
          end_date: record.end_date || '',
          status: record.status || 'pending',
        })
      }
    }
  } catch {
    ElMessage.error('加载记录失败')
  }
}

async function handleSubmit() {
  const valid = await formRef.value.validate().catch(() => false)
  if (!valid) return

  submitting.value = true
  try {
    let res
    if (isEdit.value) {
      res = await updateMaintenance({ id: Number(route.params.id), ...form })
    } else {
      res = await createMaintenance(form)
    }
    if (res.error === 0) {
      ElMessage.success(isEdit.value ? '记录已更新' : '记录已创建')
      router.push('/maintenance')
    } else {
      ElMessage.error('操作失败')
    }
  } catch {
    ElMessage.error('操作失败')
  } finally {
    submitting.value = false
  }
}

async function loadVehicles() {
  try {
    const res = await getVehicleList({ page: 1, page_size: 1000 })
    if (res.error === 0) {
      vehicles.value = (res.vehicles || []).filter(v => v.status === 'available' || v.status === 'maintenance')
    }
  } catch {
    ElMessage.error('加载车辆列表失败')
  }
}

onMounted(() => {
  loadVehicles()
  loadRecord()
})
</script>

<style scoped>
.centered {
  display: flex;
  justify-content: center;
  align-items: flex-start;
  padding-top: 80px;
}
.form-row {
  display: flex;
  gap: 20px;
}
</style>